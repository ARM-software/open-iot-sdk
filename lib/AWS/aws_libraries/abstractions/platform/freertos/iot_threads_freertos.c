/*
 * FreeRTOS Platform V1.1.2
 * Copyright (C) 2020 Amazon.com, Inc. or its affiliates.  All Rights Reserved.
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
 * @file iot_threads_freertos.c
 * @brief Implementation of the functions in iot_threads.h for POSIX systems.
 */

/* The config header is always included first. */
#include "iot_config.h"
#include "cmsis_os2.h"

/* Platform threads include. */
#include "platform/iot_platform_types_freertos.h"
#include "platform/iot_threads.h"
#include "types/iot_platform_types.h"

/* Configure logs for the functions in this file. */
#ifdef IOT_LOG_LEVEL_PLATFORM
    #define LIBRARY_LOG_LEVEL        IOT_LOG_LEVEL_PLATFORM
#else
    #ifdef IOT_LOG_LEVEL_GLOBAL
        #define LIBRARY_LOG_LEVEL    IOT_LOG_LEVEL_GLOBAL
    #else
        #define LIBRARY_LOG_LEVEL    IOT_LOG_NONE
    #endif
#endif

#define LIBRARY_LOG_NAME    ( "THREAD" )
#include "iot_logging_setup.h"

/*
 * Provide default values for undefined memory allocation functions based on
 * the usage of dynamic memory allocation.
 */
#ifndef IotThreads_Malloc
    #include "RTOS_config.h"

/**
 * @brief Memory allocation. This function should have the same signature
 * as [malloc](http://pubs.opengroup.org/onlinepubs/9699919799/functions/malloc.html).
 */
    #define IotThreads_Malloc    pvPortMalloc
#endif
#ifndef IotThreads_Free
    #include "RTOS_config.h"

/**
 * @brief Free memory. This function should have the same signature as
 * [free](http://pubs.opengroup.org/onlinepubs/9699919799/functions/free.html).
 */
    #define IotThreads_Free    pvPortFree
#endif

/*-----------------------------------------------------------*/

static void _threadRoutineWrapper( void * pArgument )
{
    threadInfo_t * pThreadInfo = ( threadInfo_t * ) pArgument;

    /* Run the thread routine. */
    pThreadInfo->threadRoutine( pThreadInfo->pArgument );
    IotThreads_Free( pThreadInfo );

    osThreadExit();
}

/*-----------------------------------------------------------*/

bool Iot_CreateDetachedThread( IotThreadRoutine_t threadRoutine,
                               void * pArgument,
                               int32_t priority,
                               size_t stackSize )
{
    bool status = true;
    osMemoryPoolId_t threadMemPoolId;

    configASSERT( threadRoutine != NULL );

    IotLogDebug( "Creating new thread." );
    threadInfo_t * pThreadInfo = IotThreads_Malloc( sizeof( threadInfo_t ) );

    if( pThreadInfo == NULL )
    {
        IotLogDebug( "Unable to allocate memory for threadRoutine %p.", threadRoutine );
        status = false;
    }

    /* Create the CMSIS task that will run the thread. */
    if( status )
    {
        pThreadInfo->threadRoutine = threadRoutine;
        pThreadInfo->pArgument = pArgument;

        const osThreadAttr_t thread_attr = {
            .name = "iot_thread", 
            .attr_bits = osThreadDetached, 
            .stack_size = (configSTACK_DEPTH_TYPE)stackSize, 
            .priority = priority, 
        };

        osThreadId_t id = osThreadNew( _threadRoutineWrapper, pThreadInfo, &thread_attr );

        if(id == NULL)
        {
            /* Task creation failed. */
            IotLogWarn( "Failed to create thread." );
            IotThreads_Free( pThreadInfo );
            status = false;
        }

        pThreadInfo->threadId = id;
    }

    return status;
}

/*-----------------------------------------------------------*/

bool IotMutex_Create( IotMutex_t * pNewMutex,
                      bool recursive )
{
    _IotSystemMutex_t * internalMutex = ( _IotSystemMutex_t * ) pNewMutex;

    configASSERT( internalMutex != NULL );

    IotLogDebug( "Creating new mutex %p.", pNewMutex );
    uint32_t mutexType;
    osMutexId_t mutex_id;

    if( recursive )
    {
        mutexType = osMutexRecursive | osMutexPrioInherit;
    }
    else
    {
        mutexType = osMutexPrioInherit;
    }

    osMutexAttr_t mutexAttr = {"mutex", mutexType, NULL, 0};

    mutex_id = osMutexNew(&mutexAttr);
    if( mutex_id != NULL )
    {
        *pNewMutex = mutex_id;
        return true;
    }

    return false;
}

/*-----------------------------------------------------------*/

void IotMutex_Destroy( IotMutex_t * pMutex )
{
    _IotSystemMutex_t * internalMutex = ( _IotSystemMutex_t * ) pMutex;

    configASSERT( internalMutex != NULL );

    osMutexDelete( * pMutex );
}

/*-----------------------------------------------------------*/

bool prIotMutexTimedLock( IotMutex_t * pMutex,
                          TickType_t timeout )
{
    _IotSystemMutex_t * internalMutex = ( _IotSystemMutex_t * ) pMutex;
    osStatus_t lockResult;

    configASSERT( internalMutex != NULL );

    IotLogDebug( "Locking mutex %p.", internalMutex );

    lockResult = osMutexAcquire( * pMutex, timeout );

    return ( lockResult == osOK );
}

/*-----------------------------------------------------------*/

void IotMutex_Lock( IotMutex_t * pMutex )
{
    prIotMutexTimedLock( pMutex, osWaitForever );
}

/*-----------------------------------------------------------*/

bool IotMutex_TryLock( IotMutex_t * pMutex )
{
    return prIotMutexTimedLock( pMutex, 0 );
}

/*-----------------------------------------------------------*/

void IotMutex_Unlock( IotMutex_t * pMutex )
{
    _IotSystemMutex_t * internalMutex = ( _IotSystemMutex_t * ) pMutex;

    configASSERT( internalMutex != NULL );

    IotLogDebug( "Unlocking mutex %p.", internalMutex );

    osStatus_t lockResult = osMutexRelease( * pMutex );
    if ( lockResult != osOK ) {
        IotLogError( "Failed to unlock mutex %p.", internalMutex );
    }
}

/*-----------------------------------------------------------*/

bool IotSemaphore_Create( IotSemaphore_t *pNewSemaphore, uint32_t initialValue, uint32_t maxValue )
{
    _IotSystemSemaphore_t * internalSemaphore = ( _IotSystemSemaphore_t * ) pNewSemaphore;

    configASSERT( internalSemaphore != NULL );

    osSemaphoreId_t id;

    IotLogDebug( "Creating new semaphore %p.", pNewSemaphore );

    id = osSemaphoreNew( maxValue, initialValue, NULL );

    if( id != NULL )
    {
        *pNewSemaphore = id;
        return true;
    }
    return false;
}

/*-----------------------------------------------------------*/

uint32_t IotSemaphore_GetCount( IotSemaphore_t * pSemaphore )
{
    _IotSystemSemaphore_t * internalSemaphore = ( _IotSystemSemaphore_t * ) pSemaphore;
    uint32_t count = 0;

    configASSERT( internalSemaphore != NULL );

    count = osSemaphoreGetCount( * internalSemaphore );

    IotLogDebug( "Semaphore %p has count %d.", pSemaphore, count );

    return count;
}

/*-----------------------------------------------------------*/

void IotSemaphore_Destroy( IotSemaphore_t * pSemaphore )
{
    _IotSystemSemaphore_t * internalSemaphore = ( _IotSystemSemaphore_t * ) pSemaphore;

    configASSERT( internalSemaphore != NULL );

    IotLogDebug( "Destroying semaphore %p.", internalSemaphore );

    osSemaphoreDelete( internalSemaphore );
}

/*-----------------------------------------------------------*/

void IotSemaphore_Wait( IotSemaphore_t * pSemaphore )
{
    _IotSystemSemaphore_t * internalSemaphore = ( _IotSystemSemaphore_t * ) pSemaphore;

    configASSERT( internalSemaphore != NULL );

    IotLogDebug( "Waiting on semaphore %p.", internalSemaphore );

    /* Take the semaphore using the FreeRTOS API. */
    if ( osSemaphoreAcquire( *internalSemaphore, osWaitForever ) != osOK ) {
        IotLogWarn( "Failed to wait on semaphore %p.", pSemaphore );

        /* Assert here, debugging we always want to know that this happened because you think
         *   that you are waiting successfully on the semaphore but you are not   */
        configASSERT( false );
    }
}

/*-----------------------------------------------------------*/

bool IotSemaphore_TryWait( IotSemaphore_t * pSemaphore )
{
    _IotSystemSemaphore_t * internalSemaphore = ( _IotSystemSemaphore_t * ) pSemaphore;

    configASSERT( internalSemaphore != NULL );

    IotLogDebug( "Attempting to wait on semaphore %p.", internalSemaphore );

    return IotSemaphore_TimedWait( pSemaphore, 0 );
}

/*-----------------------------------------------------------*/

bool IotSemaphore_TimedWait( IotSemaphore_t * pSemaphore,
                             uint32_t timeoutMs )
{
    _IotSystemSemaphore_t * internalSemaphore = ( _IotSystemSemaphore_t * ) pSemaphore;

    configASSERT( internalSemaphore != NULL );

    /* Take the semaphore using the CMSIS RTOS API. */
    if ( osSemaphoreAcquire( * internalSemaphore, timeoutMs) != osOK ) {
        /* Only warn if timeout > 0 */
        if ( timeoutMs > 0 ) {
            IotLogWarn( "Timeout waiting on semaphore %p.", internalSemaphore );
        }

        return false;
    }

    return true;
}

/*-----------------------------------------------------------*/

void IotSemaphore_Post( IotSemaphore_t * pSemaphore )
{
    _IotSystemSemaphore_t * internalSemaphore = ( _IotSystemSemaphore_t * ) pSemaphore;

    configASSERT( internalSemaphore != NULL );

    IotLogDebug( "Posting to semaphore %p.", internalSemaphore );
    /* Give the semaphore using the CMSIS RTOS API. */
    osStatus_t result = osSemaphoreRelease( * internalSemaphore );

    if ( result == osErrorResource ) {
        IotLogDebug( "Unable to give semaphore over maximum", internalSemaphore );
    } else if ( result == osErrorParameter ) {
        IotLogDebug( "Semaphore is NULL of invalid", internalSemaphore );
    }
}

/*-----------------------------------------------------------*/
