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
 * @file iot_platform_types_freertos.h
 * @brief Definitions of platform layer types on POSIX systems.
 */

#ifndef _IOT_PLATFORM_TYPES_AFR_H_
#define _IOT_PLATFORM_TYPES_AFR_H_

#include "cmsis_os2.h"

/**
 * @brief The native mutex type on AFR systems.
 */
typedef osMutexId_t _IotSystemMutex_t;

/**
 * @brief The native semaphore type on AFR systems.
 */
typedef osSemaphoreId_t _IotSystemSemaphore_t;

/**
 * @brief Holds information about an active detached thread so that we can
 *        delete the task when it completes
 */
typedef struct threadInfo {
    void *pArgument;                  /**< @brief Argument to `threadRoutine`. */
    void (*threadRoutine)(void *);    /**< @brief Thread function to run. */
    osThreadId_t threadId;            /**< Thread ID. */
} threadInfo_t;

typedef void ( * callback_t )( void * );

/**
 * @brief Represents an #IotTimer_t on AFR systems.
 */
typedef struct _IotSystemTimer_t {
    osTimerId_t timerId;
    callback_t callback;
    void *callbackArg;
} _IotSystemTimer_t;

#endif /* ifndef _IOT_PLATFORM_TYPES_POSIX_H_ */
