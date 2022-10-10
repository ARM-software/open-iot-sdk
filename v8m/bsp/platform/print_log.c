/*
 * Copyright (c) 2018-2022 Arm Limited. All Rights Reserved.
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
 */

#include <stdio.h>
#include <stdarg.h>

#include "cmsis_os2.h"
#include "print_log.h"

osMutexId_t xUARTMutex __attribute__(( section( "tasks_share" ) )) = NULL;

#if( configSUPPORT_STATIC_ALLOCATION == 1 )
osMutexId_t xUARTMutexBuffer;
#endif

//extern BaseType_t xPortRaisePrivilege( void );
//extern void vPortResetPrivilege( BaseType_t xRunningPrivileged );

#if( configSUPPORT_DYNAMIC_ALLOCATION == 1 )
/*
 * @brief Dynamically create an instance of UART mutex, and return
 *		  the mutex id.
 *
 * @return If the mutex is successfully created then the id of the created
 *		   mutex is returned. Otherwise, 0 is returned.
 */
static osMutexId_t prvCreateUARTMutex( void )
{
	osMutexId_t xMutexHandle = 0;
	const osMutexAttr_t Mutex_attr = {
		"UARTMutex",                          // human readable mutex name
		osMutexPrioInherit,    // attr_bits
		NULL,                                     // memory for control block   
		0U                                        // size for control block
	};
	xMutexHandle = osMutexNew( &Mutex_attr );
	configASSERT( (uint32_t) xMutexHandle );
	return xMutexHandle;
}
/*-----------------------------------------------------------*/

#elif( configSUPPORT_STATIC_ALLOCATION == 1 )
/*
 * @brief Statically create an instance of UART mutex, and return
 *		  the mutex id.
 *
 * @return If the mutex is successfully created then the id of the created
 *		   mutex is returned. Otherwise, 0 is returned.
 */
static osMutexId_t prvCreateUARTMutex( void )
{
	osMutexId_t xMutexHandle = 0;
	const osMutexAttr_t Mutex_attr = {
		"UARTMutex",                          // human readable mutex name
		osMutexPrioInherit,    // attr_bits
		&xUARTMutexBuffer,                                     // memory for control block   
		sizeof(osMutexId_t)                                        // size for control block
	};
	xMutexHandle = osMutexNew( Mutex_attr );
	configASSERT( xMutexHandle );
	return xMutexHandle;
}
/*-----------------------------------------------------------*/

#endif

void vUARTLockInit( void )
{
	xUARTMutex = prvCreateUARTMutex();
}
/*-----------------------------------------------------------*/

/*
 * @brief Obtain the UART mutex.
 *
 * @param xBlockTime The time in ticks to wait for the mutex to become
 *					 available.  The macro portTICK_PERIOD_MS can be used to
 *					 convert this to a real time. A block time of zero can be
 *					 used to poll the semaphore.  A block time of osWaitForever
 *					 can be used to block indefinitely.
 *
 * @return osOK if the mutex was obtained. the returned status if the mutex couldn't
 * 		   be acquired.
 */
static osStatus_t xUARTLockAcquire( uint32_t xBlockTime )
{
	return osMutexAcquire( xUARTMutex, xBlockTime );
}
/*-----------------------------------------------------------*/

/*
 * @brief Release the UART mutex previously obtained.
 *
 * @return osOK if the mutex was released.  the returned status if an error
 * 		   occurred.
 */
static osStatus_t xUARTLockRelease( void )
{
	return osMutexRelease( xUARTMutex );
}
/*-----------------------------------------------------------*/

void print_log( const char *format, ... )
{
va_list args;
	/* A UART lock is used here to ensure that there is
	 * at most one task accessing UART at a time.
	 */
	xUARTLockAcquire( osWaitForever );

	/* Raise to privilidge mode to access related data and device. */
	//xRunningPrivileged = xPortRaisePrivilege();
	va_start( args, format );
	vprintf( format, args );
	va_end ( args );
	printf("\r\n");
	xUARTLockRelease();
}
// void print_log( const char *format, ... )
// {

// }
/*-----------------------------------------------------------*/
