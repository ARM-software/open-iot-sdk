/*
 * Copyright (c) 2018-2019 Arm Limited. All Rights Reserved.
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

#include "FreeRTOS.h"
#include "print_log.h"
#include "mpu_wrappers.h"

SemaphoreHandle_t xUARTMutex __attribute__(( section( "tasks_share" ) )) = NULL;

#if( configSUPPORT_STATIC_ALLOCATION == 1 )
StaticSemaphore_t xUARTMutexBuffer;
#endif

//extern BaseType_t xPortRaisePrivilege( void );
//extern void vPortResetPrivilege( BaseType_t xRunningPrivileged );

#if( configSUPPORT_DYNAMIC_ALLOCATION == 1 )
/*
 * @brief Dynamically create an instance of UART mutex, and return
 *		  a handle to the mutex.
 *
 * @return If the mutex is successfully created then a handle to the created
 *		   mutex is returned. Otherwise, NULL is returned.
 */
static SemaphoreHandle_t prvCreateUARTMutex( void )
{
SemaphoreHandle_t xMutexHandle;
	xMutexHandle = xSemaphoreCreateMutex();
	configASSERT( xMutexHandle );
	return xMutexHandle;
}
/*-----------------------------------------------------------*/

#elif( configSUPPORT_STATIC_ALLOCATION == 1 )
/*
 * @brief Statically create an instance of UART mutex, and return
 *		  a handle to the mutex.
 *
 * @return If the mutex is successfully created then a handle to the created
 *		   mutex is returned. Otherwise, NULL is returned.
 */
static SemaphoreHandle_t prvCreateUARTMutex( void )
{
SemaphoreHandle_t xMutexHandle;
	xMutexHandle = xSemaphoreCreateMutexStatic( &xUARTMutexBuffer );
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
 *					 used to poll the semaphore.  A block time of portMAX_DELAY
 *					 can be used to block indefinitely (provided
 *					 INCLUDE_vTaskSuspend is set to 1 in FreeRTOSConfig.h).
 *
 * @return pdTRUE if the mutex was obtained. pdFALSE if xBlockTime expired
 *		   without the mutex becoming available.
 */
static BaseType_t xUARTLockAcquire( TickType_t xBlockTime )
{
	return xSemaphoreTake( xUARTMutex, xBlockTime );
}
/*-----------------------------------------------------------*/

/*
 * @brief Release the UART mutex previously obtained.
 *
 * @return pdTRUE if the mutex was released.  pdFALSE if an error occurred.
 */
static BaseType_t xUARTLockRelease( void )
{
	return xSemaphoreGive( xUARTMutex );
}
/*-----------------------------------------------------------*/

void print_log( const char *format, ... )
{
va_list args;
//BaseType_t xRunningPrivileged;

	/* A UART lock is used here to ensure that there is
	 * at most one task accessing UART at a time.
	 */
	xUARTLockAcquire( portMAX_DELAY );

	/* Raise to privilidge mode to access related data and device. */
	//xRunningPrivileged = xPortRaisePrivilege();
	va_start( args, format );
	vprintf( format, args );
	va_end ( args );
	printf("\r\n");
	/* Reset privilidge mode if it is raised. */
	//vPortResetPrivilege( xRunningPrivileged );
	xUARTLockRelease();
}
// void print_log( const char *format, ... )
// {

// }
/*-----------------------------------------------------------*/
