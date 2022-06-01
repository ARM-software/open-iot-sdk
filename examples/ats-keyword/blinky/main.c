/* Copyright (c) 2017-2022, Arm Limited and Contributors. All rights reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdio.h>
#include <stdlib.h>
#include "tfm_ns_interface.h"
#include "bsp_serial.h"

#include "cmsis_os2.h"
#include "mps3_io.h"

extern uint32_t tfm_ns_interface_init(void);

static void app_task(void *arg)
{
    (void)arg;

    mps3_io_t led_gpio;
    mps3_io_init(&led_gpio, &MPS3_IO_DEV_NS, LED1);

    printf("The LED started blinking...\r\n");
    while (1) {
        mdh_gpio_write(&led_gpio.gpio, 1U);
        printf("LED on\r\n");
        osDelay(1000u);
        mdh_gpio_write(&led_gpio.gpio, 0U);
        printf("LED off\r\n");
        osDelay(1000u);
    }
}

int main()
{
    bsp_serial_init();

    uint32_t ret = tfm_ns_interface_init();
    if (ret != 0) {
        printf("tfm_ns_interface_init() failed: %u\r\n", ret);
        return EXIT_FAILURE;
    }

    printf("Initialising kernel\r\n");
    osStatus_t os_status = osKernelInitialize();
    if (os_status != osOK) {
        printf("osKernelInitialize failed: %d\r\n", os_status);
        return EXIT_FAILURE;
    }

    osThreadId_t app_thread = osThreadNew(app_task, NULL, NULL);
    if (!app_thread) {
        printf("Failed to create app thread\r\n");
        return EXIT_FAILURE;
    }

    osKernelState_t os_state = osKernelGetState();
    if (os_state != osKernelReady) {
        printf("Kernel not ready %d\r\n", os_state);
        return EXIT_FAILURE;
    }

    printf("Starting kernel and threads\r\n");
    os_status = osKernelStart();
    if (os_status != osOK) {
        printf("Failed to start kernel: %d\r\n", os_status);
        return EXIT_FAILURE;
    }

    return 0;
}
