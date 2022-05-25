/*
 * Copyright (c) 2022, Arm Limited and Contributors. All rights reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef RTOS_CONFIG_H
#define RTOS_CONFIG_H

#include <stddef.h>
#include "basetypes.h"

#define configPRINT_STRING(X) print_log(X) // used by vLoggingPrintf
#define configPRINT(X)        print_log X  // used by dumb logging
#define configPRINTF(X)       print_log X  // used by dumb logging

/* Memory allocation related definitions. */
#define configSUPPORT_STATIC_ALLOCATION  1
#define configSUPPORT_DYNAMIC_ALLOCATION 1
#define configTOTAL_HEAP_SIZE            512000 // should be enough
#define configAPPLICATION_ALLOCATED_HEAP 1

#define configENABLE_FPU               0
#define configENABLE_MPU               0
#define configENABLE_TRUSTZONE         0
#define configRUN_FREERTOS_SECURE_ONLY 0

/* Sets the length of the buffers into which logging messages are written - so
 * also defines the maximum length of each log message. */
#define configLOGGING_MAX_MESSAGE_LENGTH 200

/* Set to 1 to prepend each log message with a message number, the task name,
 * and a time stamp. */
#define configLOGGING_INCLUDE_TIME_AND_TASK_NAME 1

// somehow 300 tick per second gives similar timing (~85%) as 1000 did on the FPGA, so with this 1 ms to 1 tick can be
// kept...
#define configTICK_RATE_HZ       ((uint32_t)300) /* Scheduler polling rate of 1000 Hz */
#define configMINIMAL_STACK_SIZE 4096
#define configUSE_16_BIT_TICKS   0
#define portTICK_TYPE_IS_ATOMIC  1

#define democonfigDEMO_STACKSIZE (2 * configMINIMAL_STACK_SIZE)
#define democonfigDEMO_PRIORITY  osPriorityNormal

#define configSTACK_DEPTH_TYPE           uint32_t
#define configMESSAGE_BUFFER_LENGTH_TYPE size_t

#define pdMS_TO_TICKS(ms) (((uint64_t)ms * osKernelGetTickFreq()) / 1000)

// Prevent redefinition if this file is included by FreeRTOS itself
#ifndef FREERTOS_CONFIG_H
#define pdFALSE ((BaseType_t)0)
#define pdTRUE  ((BaseType_t)1)

#define pdPASS pdTRUE
#define pdFAIL pdFALSE
#endif

extern void configASSERT(uint32_t condition);
extern void print_log(const char *format, ...);

extern void *pvPortMalloc(size_t size);
extern void vPortFree(void *p);

#endif // RTOS_CONFIG_H
