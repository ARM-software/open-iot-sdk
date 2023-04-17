/* Copyright (c) 2017-2022, Arm Limited and Contributors. All rights reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "RTOS_config.h"
#include "blink_task.h"
#include "bsp_serial.h"
#include "cmsis_os2.h"
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
}

void OTA_HookStop(void)
{
    ml_task_inference_start();
}

void main_task(void *arg)
{
    (void)arg;

    tfm_ns_interface_init();

    vUARTLockInit();

    static const osThreadAttr_t ml_task_attr = {.priority = osPriorityHigh, .stack_size = 8192, .name = "ML_TASK"};
    osThreadId_t ml_thread = osThreadNew(ml_task, NULL, &ml_task_attr);
    if (!ml_thread) {
        printf("Failed to create ml thread\r\n");
        return;
    }

    static const osThreadAttr_t blink_attr = {.priority = osPriorityNormal, .name = "BLINK_TASK"};
    osThreadId_t blink_thread = osThreadNew(blink_task, NULL, &blink_attr);
    if (!blink_thread) {
        printf("Failed to create blink thread\r\n");
        return;
    }

    static const osThreadAttr_t ml_mqtt_attr = {.priority = osPriorityNormal, .name = "ML_MQTT"};
    osThreadId_t ml_mqtt_thread = osThreadNew(ml_mqtt_task, NULL, &ml_mqtt_attr);
    if (!ml_mqtt_thread) {
        printf("Failed to create ml mqtt thread\r\n");
        return;
    }

    mbedtls_platform_set_calloc_free(prvCalloc, free);

    if (endpoint_init()) {
        printf("Failed to start endpoint task\r\n");
    }

    while (1) {
        osDelay(osWaitForever);
    };
    return;
}

int main()
{
    bsp_serial_init();

    osStatus_t os_status = osKernelInitialize();
    if (os_status != osOK) {
        printf("osKernelInitialize failed: %d\r\n", os_status);
        return EXIT_FAILURE;
    }

    static const osThreadAttr_t main_task_attr = {.priority = osPriorityNormal, .name = "main_task"};
    osThreadId_t connectivity_thread = osThreadNew(main_task, NULL, &main_task_attr);
    if (!connectivity_thread) {
        printf("Failed to create connectivity thread\r\n");
        return EXIT_FAILURE;
    }

    osKernelState_t os_state = osKernelGetState();
    if (os_state != osKernelReady) {
        printf("Kernel not ready %d\r\n", os_state);
        return EXIT_FAILURE;
    }

    printf("Starting scheduler from ns main\r\n");

    /* Start the scheduler itself. */
    os_status = osKernelStart();
    if (os_status != osOK) {
        printf("Failed to start kernel: %d\r\n", os_status);
        return EXIT_FAILURE;
    }

    return 0;
}
