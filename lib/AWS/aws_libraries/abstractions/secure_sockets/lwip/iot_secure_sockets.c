/*
 * FreeRTOS Secure Sockets V1.3.1
 * Copyright (C) 2021 Amazon.com, Inc. or its affiliates.  All Rights Reserved.
 * Copyright (c) 2022, Arm Limited and Contributors. All rights reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy of
 * this software and associated documentation files (the "Software"), to deal in
 * the Software without restriction, including without limitation the rights to
 * use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of
 * the Software, and to permit persons to whom the Software is furnished to do so,
 * subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
 * FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
 * COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
 * IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 * http://aws.amazon.com/freertos
 * http://www.FreeRTOS.org
 */

/**
 * @file iot_secure_sockets.c
 * @brief Secure Socket interface implementation.
 */

/* Secure Socket interface includes. */
#include "cmsis_os2.h"
#include "aws_secure_sockets_config.h"
#include "iot_secure_sockets.h"
#include "iot_socket.h"
#include "iot_config.h"

#include "iot_tls.h"

#include <string.h>
#include <stdbool.h>

#include "bootstrap/mbed_atomic.h"

/*-----------------------------------------------------------*/

#define SS_STATUS_CONNECTED               ( 1 )
#define SS_STATUS_SECURED                 ( 2 )

#define SECURE_SOCKETS_SELECT_WAIT_SEC    ( 10 )

#define SOCKETS_START_DELETION            ( 0x01 )
#define SOCKETS_COMPLETE_DELETION         ( 0x02 )

/*
 * secure socket context.
 */
typedef enum E_AWS_SOCkET_RX_STATE
{
    SST_RX_IDLE,
    SST_RX_READY,
    SST_RX_CLOSING,
    SST_RX_CLOSED,
} T_AWS_SOCKET_RX_STATE;

typedef struct _ss_ctx_t
{
    int ip_socket;

    int state;
    unsigned int status;
    int send_flag;
    int recv_flag;

    osThreadId_t rx_handle;
    void ( * rx_callback )( Socket_t pxSocket );
    osEventFlagsId_t rx_EventFlags;

    bool enforce_tls;
    void * tls_ctx;
    char * destination;

    char * server_cert;
    int server_cert_len;

    char ** ppcAlpnProtocols;
    uint32_t ulAlpnProtocolsCount;
    uint32_t ulRefcount;
} ss_ctx_t;

/*-----------------------------------------------------------*/

/*#define SUPPORTED_DESCRIPTORS  (2) */

/*-----------------------------------------------------------*/

/*static int8_t sockets_allocated = SUPPORTED_DESCRIPTORS; */
static int8_t sockets_allocated = socketsconfigDEFAULT_MAX_NUM_SECURE_SOCKETS;


/*-----------------------------------------------------------*/

/*
 * convert from system ticks to seconds.
 */
#define TICK_TO_S( _t_ )     ( ( _t_ ) / configTICK_RATE_HZ )

/*
 * convert from system ticks to micro seconds.
 */
#define TICK_TO_US( _t_ )    ( ( _t_ ) * 1000 / configTICK_RATE_HZ * 1000 )

/*-----------------------------------------------------------*/

/*
 * @brief Sockets close
 */
static void prvSocketsClose( ss_ctx_t * ctx )
{
    uint32_t ulProtocol;

    sockets_allocated++;

    /* Clean-up application protocol array. */
    if( NULL != ctx->ppcAlpnProtocols )
    {
        for( ulProtocol = 0;
             ulProtocol < ctx->ulAlpnProtocolsCount;
             ulProtocol++ )
        {
            if( NULL != ctx->ppcAlpnProtocols[ ulProtocol ] )
            {
                vPortFree( ctx->ppcAlpnProtocols[ ulProtocol ] );
            }
        }

        vPortFree( ctx->ppcAlpnProtocols );
    }

    if( true == ctx->enforce_tls )
    {
        TLS_Cleanup( ctx->tls_ctx );
    }

    if( ctx->server_cert )
    {
        vPortFree( ctx->server_cert );
    }

    if( ctx->destination )
    {
        vPortFree( ctx->destination );
    }

    vPortFree( ctx );
}

/*
 * @brief Decrement ctx refcount and call release function if the count is 1 (
 *        last user of the ctx)
 */
static void prvDecrementRefCount( ss_ctx_t * ctx )
{
    if ( core_util_atomic_decr_u32( &ctx->ulRefcount, 1 ) ) {
        prvSocketsClose( ctx );
    }
}

/*
 * @brief Increment ctx refcount
 */
static void prvIncrementRefCount( ss_ctx_t * ctx )
{
    core_util_atomic_incr_u32( &ctx->ulRefcount, 1 );
}

/*
 * @brief Network send callback.
 */
static BaseType_t prvNetworkSend( void * pvContext,
                                  const unsigned char * pucData,
                                  size_t xDataLength )
{
    ss_ctx_t * ctx = ( ss_ctx_t * ) pvContext;

    int ret = iotSocketSend( ctx->ip_socket, pucData, xDataLength );

    return ( BaseType_t ) ret;
}

/*-----------------------------------------------------------*/

/*
 * @brief Network receive callback.
 */
static BaseType_t prvNetworkRecv( void * pvContext,
                                  unsigned char * pucReceiveBuffer,
                                  size_t xReceiveLength )
{
    ss_ctx_t * ctx;

    ctx = ( ss_ctx_t * ) pvContext;

    configASSERT( ctx->ip_socket >= 0 );

    int ret = iotSocketRecv( ctx->ip_socket, pucReceiveBuffer, xReceiveLength );

    switch( ret ) {
        case IOT_SOCKET_EINVAL:
            return SOCKETS_EINVAL;
        case IOT_SOCKET_ENOTCONN:
            return SOCKETS_ENOTCONN;
        case IOT_SOCKET_ECONNRESET:
        case IOT_SOCKET_ECONNABORTED:
            return SOCKETS_ECLOSED;
        case IOT_SOCKET_EAGAIN:
            return SOCKETS_ERROR_NONE; /* timeout or would block */
        case IOT_SOCKET_ERROR:
        case IOT_SOCKET_ESOCK:
            return SOCKETS_SOCKET_ERROR;
        case 0:
            if( ( 0 == ret ) && ( xReceiveLength != 0 ) )
            {
                ret = SOCKETS_ECLOSED;
            }
    }

    return ( BaseType_t ) ret;
}

/*-----------------------------------------------------------*/

static void vTaskRxSelect( void * param )
{
    ss_ctx_t * ctx = ( ss_ctx_t * ) param;
    int s = ctx->ip_socket;
    ctx->state = SST_RX_READY;

    while( 1 )
    {
        if( ctx->state == SST_RX_CLOSING )
        {
            ctx->state = SST_RX_CLOSED;
            break;
        }

        uint32_t ret;
        for ( ; ; )
        {
            ret = iotSocketRecv( s, NULL, 0 );
            if ( ret != IOT_SOCKET_EAGAIN )
            {
                break;
            }
        }

        if (ret == 0)
        {
            ctx->rx_callback( ( Socket_t ) ctx );
        }
        else
        {
            break;
        }

        if( osEventFlagsWait( ctx->rx_EventFlags, SOCKETS_START_DELETION, osFlagsWaitAll, 0 ) == SOCKETS_START_DELETION )
        {
            /* Inform the main task that the event has been received. */
            ( void ) osEventFlagsSet( ctx->rx_EventFlags, SOCKETS_COMPLETE_DELETION );
            break;
        }
    }

    prvDecrementRefCount( ctx );

    osThreadExit();
}


/*-----------------------------------------------------------*/

static void prvRxSelectSet( ss_ctx_t * ctx,
                            const void * pvOptionValue )
{
    BaseType_t xReturned;

    ctx->rx_callback = ( void ( * )( Socket_t ) )pvOptionValue;

    prvIncrementRefCount( ctx );

    ctx->rx_EventFlags = osEventFlagsNew( NULL );

    configASSERT( ctx->rx_EventFlags != NULL );

    osThreadAttr_t attr = {
      .stack_size = socketsconfigRECEIVE_CALLBACK_TASK_STACK_DEPTH,
      .priority = osPriorityNormal
    };

    ctx->rx_handle = osThreadNew( vTaskRxSelect, ctx, &attr);

    configASSERT( ctx->rx_handle != NULL );
}

/*-----------------------------------------------------------*/

static void prvRxSelectClear( ss_ctx_t * ctx )
{
    /* Inform the vTaskRxSelect to delete itself. */
    osEventFlagsSet( ctx->rx_EventFlags, SOCKETS_START_DELETION );

    /* Wait for the task to delete itself. */
    while( osEventFlagsWait( ctx->rx_EventFlags, SOCKETS_COMPLETE_DELETION, osFlagsWaitAll, osWaitForever )
           != SOCKETS_COMPLETE_DELETION )
    {
        /* Continue waiting for the task to delete itself. */
    }

    /* Reset the handle of the task to NULL. */
    ctx->rx_handle = NULL;

    /* Delete the event group. */
    osEventFlagsDelete( ctx->rx_EventFlags );

    /* Remove the reference to the callback. */
    ctx->rx_callback = NULL;
}

/*-----------------------------------------------------------*/

Socket_t SOCKETS_Socket( int32_t lDomain,
                         int32_t lType,
                         int32_t lProtocol )
{
    ss_ctx_t * ctx;

    configASSERT( lDomain == SOCKETS_AF_INET );
    configASSERT( lType == SOCKETS_SOCK_STREAM );
    configASSERT( lProtocol == SOCKETS_IPPROTO_TCP );

    if( ( lDomain != SOCKETS_AF_INET ) ||
        ( lType != SOCKETS_SOCK_STREAM ) ||
        ( lProtocol != SOCKETS_IPPROTO_TCP ) ||
        ( sockets_allocated <= 0 )
        )
    {
        return SOCKETS_INVALID_SOCKET;
    }

    ctx = ( ss_ctx_t * ) pvPortMalloc( sizeof( *ctx ) );

    if( ctx )
    {
        memset( ctx, 0, sizeof( *ctx ) );

        ctx->ip_socket = iotSocketCreate( IOT_SOCKET_AF_INET, IOT_SOCKET_SOCK_STREAM, IOT_SOCKET_IPPROTO_TCP );

        if( ctx->ip_socket >= 0 )
        {
            ctx->ulRefcount = 1;
            sockets_allocated--;
            return ( Socket_t ) ctx;
        }

        vPortFree( ctx );
    }

    return ( Socket_t ) SOCKETS_INVALID_SOCKET;
}

/*-----------------------------------------------------------*/

int32_t SOCKETS_Bind( Socket_t xSocket,
                      SocketsSockaddr_t * pxAddress,
                      Socklen_t xAddressLength )
{
    ss_ctx_t * ctx;
    int32_t ret;

    if( SOCKETS_INVALID_SOCKET == xSocket )
    {
        configPRINTF( ( "TCP socket Invalid\n" ) );
        return SOCKETS_EINVAL;
    }

    if( pxAddress == NULL )
    {
        configPRINTF( ( "TCP socket Invalid Address\n" ) );
        return SOCKETS_EINVAL;
    }

    ctx = ( ss_ctx_t * ) xSocket;

    if( NULL == ctx )
    {
        configPRINTF( ( "Invalid secure socket passed: Socket=%p\n", ctx ) );
        return SOCKETS_EINVAL;
    }

    if( 0 > ctx->ip_socket )
    {
        configPRINTF( ( "TCP socket Invalid index\n" ) );
        return SOCKETS_EINVAL;
    }

    ret = iotSocketBind( ctx->ip_socket, ( uint8_t * ) &pxAddress->ulAddress, sizeof( pxAddress->ulAddress ), pxAddress->usPort );

    if( 0 != ret )
    {
        configPRINTF( ( "iotSocketBind fail :%d\n", ret ) );
        return SOCKETS_SOCKET_ERROR;
    }

    return SOCKETS_ERROR_NONE;
}

/*-----------------------------------------------------------*/

int32_t SOCKETS_Connect( Socket_t xSocket,
                         SocketsSockaddr_t * pxAddress,
                         Socklen_t xAddressLength )
{
    ss_ctx_t * ctx;

    if( SOCKETS_INVALID_SOCKET == xSocket )
    {
        return SOCKETS_EINVAL;
    }

    /* removed because qualification program wants invalid length to go through */
    #if 0
        if( ( NULL == pxAddress ) || ( 0 == xAddressLength ) )
        {
            return SOCKETS_EINVAL;
        }
    #endif

    if( pxAddress == NULL )
    {
        return SOCKETS_EINVAL;
    }

    /* support only SOCKETS_AF_INET for now */
    pxAddress->ucSocketDomain = SOCKETS_AF_INET;

    ctx = ( ss_ctx_t * ) xSocket;
    configASSERT( ctx->ip_socket >= 0 );

    int ret;

    ret = iotSocketConnect( ctx->ip_socket, ( uint8_t * ) &pxAddress->ulAddress, sizeof( pxAddress->ulAddress ), SOCKETS_ntohs(pxAddress->usPort) );

    if( 0 == ret )
    {
        TLSParams_t tls_params = { 0 };
        BaseType_t status;

        ctx->status |= SS_STATUS_CONNECTED;

        if( !ctx->enforce_tls )
        {
            return SOCKETS_ERROR_NONE;
        }

        tls_params.ulSize = sizeof( tls_params );
        tls_params.pcDestination = ctx->destination;
        tls_params.pcServerCertificate = ctx->server_cert;
        tls_params.ulServerCertificateLength = ctx->server_cert_len;
        tls_params.pvCallerContext = ctx;
        tls_params.pxNetworkRecv = prvNetworkRecv;
        tls_params.pxNetworkSend = prvNetworkSend;
        tls_params.ppcAlpnProtocols = ( const char ** ) ctx->ppcAlpnProtocols;
        tls_params.ulAlpnProtocolsCount = ctx->ulAlpnProtocolsCount;

        status = TLS_Init( &ctx->tls_ctx, &tls_params );

        if( osOK != status )
        {
            configPRINTF( ( "TLS_Init fail\n" ) );
            return SOCKETS_SOCKET_ERROR;
        }

        status = TLS_Connect( ctx->tls_ctx );

        if( osOK == status )
        {
            ctx->status |= SS_STATUS_SECURED;
            return SOCKETS_ERROR_NONE;
        }
        else
        {
            configPRINTF( ( "TLS_Connect fail (0x%x, %s)\n",
                            ( unsigned int ) -status,
                            ctx->destination ? ctx->destination : "NULL" ) );
        }
    }
    else
    {
        configPRINTF( ( "iotSocketConnect fail %d\n", ret ) );
    }

    return SOCKETS_SOCKET_ERROR;
}

/*-----------------------------------------------------------*/

int32_t SOCKETS_Recv( Socket_t xSocket,
                      void * pvBuffer,
                      size_t xBufferLength,
                      uint32_t ulFlags )
{
    ss_ctx_t * ctx = ( ss_ctx_t * ) xSocket;

    if( SOCKETS_INVALID_SOCKET == xSocket )
    {
        return SOCKETS_SOCKET_ERROR;
    }

    if( ( NULL == pvBuffer ) || ( 0 == xBufferLength ) )
    {
        return SOCKETS_EINVAL;
    }

    if( ( ctx->status & SS_STATUS_CONNECTED ) != SS_STATUS_CONNECTED )
    {
        return SOCKETS_ENOTCONN;
    }

    ctx->recv_flag = ulFlags;

    configASSERT( ctx->ip_socket >= 0 );

    if( ctx->enforce_tls )
    {
        /* Receive through TLS pipe, if negotiated. */
        return TLS_Recv( ctx->tls_ctx, pvBuffer, xBufferLength );
    }
    else
    {
        return prvNetworkRecv( ( void * ) ctx, pvBuffer, xBufferLength );
    }
}

/*-----------------------------------------------------------*/

int32_t SOCKETS_Send( Socket_t xSocket,
                      const void * pvBuffer,
                      size_t xDataLength,
                      uint32_t ulFlags )
{
    ss_ctx_t * ctx;

    if( SOCKETS_INVALID_SOCKET == xSocket )
    {
        return SOCKETS_SOCKET_ERROR;
    }

    if( ( NULL == pvBuffer ) || ( 0 == xDataLength ) )
    {
        return SOCKETS_EINVAL;
    }

    ctx = ( ss_ctx_t * ) xSocket;

    if( ( ctx->status & SS_STATUS_CONNECTED ) != SS_STATUS_CONNECTED )
    {
        return SOCKETS_ENOTCONN;
    }

    configASSERT( ctx->ip_socket >= 0 );
    ctx->send_flag = ulFlags;

    if( ctx->enforce_tls )
    {
        /* Send through TLS pipe, if negotiated. */
        return TLS_Send( ctx->tls_ctx, pvBuffer, xDataLength );
    }
    else
    {
        return prvNetworkSend( ( void * ) ctx, pvBuffer, xDataLength );
    }
}

/*-----------------------------------------------------------*/

int32_t SOCKETS_Shutdown( Socket_t xSocket,
                          uint32_t ulHow )
{
    ss_ctx_t * ctx;
    int ret;
    (void)ulHow;

    if( SOCKETS_INVALID_SOCKET == xSocket )
    {
        return SOCKETS_EINVAL;
    }

    ctx = ( ss_ctx_t * ) xSocket;

    configASSERT( ctx->ip_socket >= 0 );
    /* API doesn't offer a shutdown call so we use close.
     * Internally, shutdown of both write&read (which is what is called by existing code) is equivalent to close. */
    iotSocketClose( ctx->ip_socket );

    if( 0 != ret )
    {
        return SOCKETS_SOCKET_ERROR;
    }

    return SOCKETS_ERROR_NONE;
}

/*-----------------------------------------------------------*/

int32_t SOCKETS_Close( Socket_t xSocket )
{
    ss_ctx_t * ctx;

    if( SOCKETS_INVALID_SOCKET == xSocket )
    {
        return SOCKETS_EINVAL;
    }

    ctx = ( ss_ctx_t * ) xSocket;
    ctx->state = SST_RX_CLOSING;

    iotSocketClose( ctx->ip_socket );
    prvDecrementRefCount( ctx );

    return SOCKETS_ERROR_NONE;
}


/*-----------------------------------------------------------*/

int32_t SOCKETS_SetSockOpt( Socket_t xSocket,
                            int32_t lLevel,
                            int32_t lOptionName,
                            const void * pvOptionValue,
                            size_t xOptionLength )
{
    ss_ctx_t * ctx;
    int ret = 0;
    char ** ppcAlpnIn = ( char ** ) pvOptionValue;
    size_t xLength = 0;
    uint32_t ulProtocol;

    if( SOCKETS_INVALID_SOCKET == xSocket )
    {
        return SOCKETS_EINVAL;
    }

    ctx = ( ss_ctx_t * ) xSocket;

    configASSERT( ctx->ip_socket >= 0 );

    switch( lOptionName )
    {
        case SOCKETS_SO_RCVTIMEO:
        case SOCKETS_SO_SNDTIMEO:
           {
               uint32_t ticks;

               ticks = *( ( const uint32_t * ) pvOptionValue );

               uint32_t timeout = ( TICK_TO_US( ticks ) ) / 1000;

               ret = iotSocketSetOpt( ctx->ip_socket,
                                      lOptionName == SOCKETS_SO_RCVTIMEO ?
                                      IOT_SOCKET_SO_RCVTIMEO : IOT_SOCKET_SO_SNDTIMEO,
                                      ( uint8_t * ) &timeout,
                                      sizeof( timeout ) );

               if( 0 != ret )
               {
                   return SOCKETS_EINVAL;
               }

               break;
           }

        case SOCKETS_SO_NONBLOCK:
           {
               uint32_t opt;

               if( ( ctx->status & SS_STATUS_CONNECTED ) != SS_STATUS_CONNECTED )
               {
                   return SOCKETS_ENOTCONN;
               }

               opt = 1;

               ret = iotSocketSetOpt( ctx->ip_socket, IOT_SOCKET_IO_FIONBIO, ( uint8_t * ) &opt, sizeof( uint32_t ) );

               if( 0 != ret )
               {
                   return SOCKETS_EINVAL;
               }

               break;
           }

        case SOCKETS_SO_REQUIRE_TLS:

            if( ctx->status & SS_STATUS_CONNECTED )
            {
                return SOCKETS_EISCONN;
            }

            ctx->enforce_tls = true;
            break;

        case SOCKETS_SO_TRUSTED_SERVER_CERTIFICATE:

            if( ctx->status & SS_STATUS_CONNECTED )
            {
                return SOCKETS_EISCONN;
            }

            if( ( NULL == pvOptionValue ) || ( 0 == xOptionLength ) )
            {
                return SOCKETS_EINVAL;
            }

            if( ctx->server_cert )
            {
                vPortFree( ctx->server_cert );
            }

            ctx->server_cert = pvPortMalloc( xOptionLength + 1 );

            if( NULL == ctx->server_cert )
            {
                return SOCKETS_ENOMEM;
            }

            memset( ctx->server_cert, 0, xOptionLength + 1 );
            memcpy( ctx->server_cert, pvOptionValue, xOptionLength );
            ctx->server_cert_len = xOptionLength;

            break;

        case SOCKETS_SO_SERVER_NAME_INDICATION:

            if( ctx->status & SS_STATUS_CONNECTED )
            {
                return SOCKETS_EISCONN;
            }

            if( ( NULL == pvOptionValue ) || ( 0 == xOptionLength ) )
            {
                return SOCKETS_EINVAL;
            }

            if( ctx->destination )
            {
                vPortFree( ctx->destination );
            }

            ctx->destination = pvPortMalloc( xOptionLength + 1 );

            if( NULL == ctx->destination )
            {
                return SOCKETS_ENOMEM;
            }

            memcpy( ctx->destination, pvOptionValue, xOptionLength );
            ctx->destination[ xOptionLength ] = '\0';

            break;

        case SOCKETS_SO_WAKEUP_CALLBACK:

            if( ( xOptionLength == sizeof( void * ) ) &&
                ( pvOptionValue != NULL ) )
            {
                prvRxSelectSet( ctx, pvOptionValue );
            }
            else
            {
                prvRxSelectClear( ctx );
            }

            break;

        case SOCKETS_SO_ALPN_PROTOCOLS:

            /* Do not set the ALPN option if the socket is already connected. */
            if( ctx->status & SS_STATUS_CONNECTED )
            {
                return SOCKETS_EISCONN;
            }

            /* Allocate a sufficiently long array of pointers. */
            ctx->ulAlpnProtocolsCount = 1 + xOptionLength;

            if( NULL == ( ctx->ppcAlpnProtocols =
                              ( char ** ) pvPortMalloc( ctx->ulAlpnProtocolsCount *
                                                        sizeof( char * ) ) ) )
            {
                return SOCKETS_ENOMEM;
            }
            else
            {
                memset( ctx->ppcAlpnProtocols,
                        0x00,
                        ctx->ulAlpnProtocolsCount * sizeof( char * ) );
            }

            /* Copy each protocol string. */
            for( ulProtocol = 0; ( ulProtocol < ctx->ulAlpnProtocolsCount - 1 );
                 ulProtocol++ )
            {
                xLength = strlen( ppcAlpnIn[ ulProtocol ] );

                if( NULL == ( ctx->ppcAlpnProtocols[ ulProtocol ] =
                                  ( char * ) pvPortMalloc( 1 + xLength ) ) )
                {
                    ctx->ppcAlpnProtocols[ ulProtocol ] = NULL;
                    return SOCKETS_ENOMEM;
                }
                else
                {
                    memcpy( ctx->ppcAlpnProtocols[ ulProtocol ],
                            ppcAlpnIn[ ulProtocol ],
                            xLength );
                    ctx->ppcAlpnProtocols[ ulProtocol ][ xLength ] = '\0';
                }
            }

            break;

        case SOCKETS_SO_TCPKEEPALIVE:

            ret = iotSocketSetOpt( ctx->ip_socket, IOT_SOCKET_SO_KEEPALIVE, pvOptionValue, sizeof( uint32_t ) );

            break;

        default:
            return SOCKETS_ENOPROTOOPT;
    }

    if( 0 > ret )
    {
        return SOCKETS_SOCKET_ERROR;
    }

    return ret;
}

/*-----------------------------------------------------------*/

uint32_t SOCKETS_GetHostByName( const char * pcHostName )
{
    uint32_t ret;
    uint32_t addr;
    /* we only want IPV4 address */
    uint32_t ip_len = sizeof( uint32_t );

    if( strlen( pcHostName ) > ( size_t ) securesocketsMAX_DNS_NAME_LENGTH ) {
         configPRINTF( ( "Host name (%s) too long!", pcHostName ) );
    }

    ret = iotSocketGetHostByName( pcHostName, IOT_SOCKET_AF_INET, ( uint8_t * ) &addr, &ip_len );

    if ( ret != 0 ) {
        configPRINTF( ( "Error (%lu) from iotSocketGetHostByName() while resolving (%s)!", ret, pcHostName ) );
        return 0;
    }

    if( addr == 0 )
    {
        configPRINTF( ( "Unable to resolve (%s)", pcHostName ) );
    }

    return addr;
}

/*-----------------------------------------------------------*/

BaseType_t SOCKETS_Init( void )
{
    /* nothing to be done, DNS is initialised as part of network init */
    return true;
}

/*-----------------------------------------------------------*/
