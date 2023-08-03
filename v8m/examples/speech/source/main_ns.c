/* Copyright (c) 2017-2022, Arm Limited and Contributors. All rights reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "RTOS_config.h"
#include "blink_task.h"
#include "cmsis_os2.h"
#include "dsp_task.h"
#include "mbedtls/platform.h"
#include "ml_interface.h"

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/*
 * Semihosting is a mechanism that enables code running on an ARM target
 * to communicate and use the Input/Output facilities of a host computer
 * that is running a debugger.
 * There is an issue where if you use armclang at -O0 optimisation with
 * no parameters specified in the main function, the initialisation code
 * contains a breakpoint for semihosting by default. This will stop the
 * code from running before main is reached.
 * Semihosting can be disabled by defining __ARM_use_no_argv symbol
 * (or using higher optimization level).
 */
#if defined(__ARMCC_VERSION) && (__ARMCC_VERSION >= 6010050)
__asm("  .global __ARM_use_no_argv\n");
#endif

extern uint32_t tfm_ns_interface_init(void);
extern void vUARTLockInit(void);
extern int endpoint_init(void);

static void *prvCalloc(size_t xNmemb, size_t xSize)
{
    void *pvNew = pvPortMalloc(xNmemb * xSize);

    if (NULL != pvNew) {
        memset(pvNew, 0, xNmemb * xSize);
    }

    return pvNew;
}

void OTA_HookStart(void)
{
    ml_task_inference_stop();
    dsp_task_stop();
}

void OTA_HookStop(void)
{
    dsp_task_start();
    ml_task_inference_start();
}

int main()
{

    tfm_ns_interface_init();

    vUARTLockInit();

    void *dspMLConnection = getDspMLConnection();

    static const osThreadAttr_t blink_attr = {.priority = osPriorityHigh, .name = "BLINK_TASK"};
    osThreadId_t blink_thread = osThreadNew(blink_task, NULL, &blink_attr);
    if (!blink_thread) {
        printf("Failed to create blink thread\r\n");
        return -1;
    }

    static const osThreadAttr_t dsp_task_attr = {
        .priority = osPriorityAboveNormal6, .stack_size = 8192, .name = "DSP_TASK"};
    osThreadId_t dsp_thread = osThreadNew(dsp_task, dspMLConnection, &dsp_task_attr);
    if (!dsp_thread) {
        printf("Failed to create dsp thread\r\n");
        return -1;
    }

    static const osThreadAttr_t ml_task_attr = {
        .priority = osPriorityAboveNormal7, .stack_size = 8192, .name = "ML_TASK"};
    osThreadId_t ml_thread = osThreadNew(ml_task, dspMLConnection, &ml_task_attr);
    if (!ml_thread) {
        printf("Failed to create ml thread\r\n");
        return -1;
    }

    static const osThreadAttr_t ml_mqtt_attr = {.priority = osPriorityNormal, .name = "ML_MQTT"};
    osThreadId_t ml_mqtt_thread = osThreadNew(ml_mqtt_task, NULL, &ml_mqtt_attr);
    if (!ml_mqtt_thread) {
        printf("Failed to create ml mqtt thread\r\n");
        return -1;
    }

    mbedtls_platform_set_calloc_free(prvCalloc, free);

    if (endpoint_init()) {
        printf("Failed to start endpoint task\r\n");
    }

    while (1) {
        osDelay(osWaitForever);
    };
    return 0;
}
