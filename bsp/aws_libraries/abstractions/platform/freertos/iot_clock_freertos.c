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
 * @file iot_clock_freertos.c
 * @brief Implementation of the functions in iot_clock.h for CMSIS systems.
 */

/* The config header is always included first. */
#include "iot_config.h"

/* Standard includes. */
#include <stdio.h>
#include "cmsis_os2.h"
#include "os_tick.h"

/* Platform clock include. */
#include "platform/iot_platform_types_freertos.h"
#include "platform/iot_clock.h"

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

#define LIBRARY_LOG_NAME    ( "CLOCK" )
#include "iot_logging_setup.h"

/*-----------------------------------------------------------*/

/*
 * Time conversion constants.
 */
#define _MILLISECONDS_PER_SECOND    ( 1000 )                                          /**< @brief Milliseconds per second. */
#define _MILLISECONDS_PER_TICK      ( _MILLISECONDS_PER_SECOND / configTICK_RATE_HZ ) /**< Milliseconds per FreeRTOS tick. */

/*-----------------------------------------------------------*/

bool IotClock_GetTimestring(char *pBuffer,
                            size_t bufferSize,
                            size_t *pTimestringLength)
{
    uint64_t milliSeconds = IotClock_GetTimeMs();
    int timestringLength = 0;

    configASSERT( pBuffer != NULL );
    configASSERT( pTimestringLength != NULL );

    /* Convert the localTime struct to a string. */
    timestringLength = snprintf( pBuffer, bufferSize, "%llu", milliSeconds );

    /* Check for error from no string */
    if( timestringLength == 0 )
    {
        return false;
    }

    /* Set the output parameter. */
    *pTimestringLength = timestringLength;

    return true;
}

/*-----------------------------------------------------------*/

uint64_t IotClock_GetTimeMs( void )
{
    static uint32_t lastCount;
    static uint64_t overflows = 0;

    /* This is called by the scheduler but also by the logger so we need a mutex. */
    /* We can do the creation here since this is guaranteed to be first called by the scheduler. */
    static mutex_id = 0;
    if (!mutex_id)
    {
        mutex_id = osMutexNew(NULL);
    }

    osMutexAcquire(mutex_id, 0);

    uint32_t tickCount = osKernelGetTickCount();

    if ( tickCount < lastCount )
    {
        overflows++;
    }

    lastCount = tickCount;

    osMutexRelease(mutex_id);

    uint64_t tickResult = tickCount;
    tickResult = tickResult + overflows * UINT32_MAX;

    /* Return the ticks converted to Milliseconds */
    return ( tickResult * _MILLISECONDS_PER_TICK );
}
/*-----------------------------------------------------------*/

void IotClock_SleepMs( uint32_t sleepTimeMs )
{
    osDelay( pdMS_TO_TICKS( sleepTimeMs ) );
}

/*-----------------------------------------------------------*/

bool IotClock_TimerCreate( IotTimer_t * pNewTimer,
                           IotThreadRoutine_t expirationRoutine,
                           void * pArgument )
{
    configASSERT( pNewTimer != NULL );
    configASSERT( expirationRoutine != NULL );

    IotLogDebug( "Delaying creation of timer %p.", pxTimer );

    /* Set the timer expiration routine, argument and period */
    pNewTimer->timerId = 0;
    pNewTimer->callback = expirationRoutine;
    pNewTimer->callbackArg = pArgument;

    return true;
}

/*-----------------------------------------------------------*/

static osTimerId_t SafeTimerDelete(osTimerId_t timerId)
{
    if( timerId != 0 )
    {
        if( osTimerDelete( timerId ) != osOK )
        {
            IotLogError( "Failed to delete timer %p.", pTimer );
        }
        else
        {
            timerId = 0;
        }
    }
    return timerId;
}

void IotClock_TimerDestroy( IotTimer_t * pTimer )
{
    if( pTimer == NULL )
    {
        IotLogError( "Timer doesn't exist. Can't destroy it." );
    }

    IotLogDebug( "Destroying timer %p.", pTimer );

    pTimer->timerId = SafeTimerDelete( pTimer->timerId );
}

/*-----------------------------------------------------------*/

bool IotClock_TimerArm( IotTimer_t * pTimer,
                        uint32_t relativeTimeoutMs,
                        uint32_t periodMs )
{
    if ( pTimer == NULL || pTimer->timerId == 0 ) {
        IotLogError( "Timer doesn't exist. Can't Arm it." );
    }

    pTimer->timerId = SafeTimerDelete( pTimer->timerId );

    osTimerType_t timer_type = ( 0 == periodMs ) ? osTimerOnce : osTimerPeriodic;

    pTimer->timerId = osTimerNew( pTimer->callback, timer_type, pTimer->callbackArg, NULL );

    if( pTimer->timerId == NULL )
    {
        IotLogError( "Failed to create timer %p.", pTimer );
    }
    else
    {
        IotLogDebug("Arming timer %p with timeout %llu.", pTimer, relativeTimeoutMs);
        if (relativeTimeoutMs != periodMs)
        {
            IotLogError( "Requested periodicity %llu couldn't be set. Period must be the same as first timeout.", relativeTimeoutMs );
        }

        osTimerStart( pTimer->timerId, pdMS_TO_TICKS( relativeTimeoutMs ) );
    }

    return true;
}

/*-----------------------------------------------------------*/
