/* Copyright (c) 2021-2023, Arm Limited and Contributors. All rights reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "blink_task.h"
#include "cmsis_os2.h"
#include "mps3_leds.h"

#include <stdio.h>

#define BLINK_TIMER_PERIOD_MS 250

enum {
    LED1 = 1 << 0,
    LED2 = 1 << 1,
    LED3 = 1 << 2,
    LED4 = 1 << 3,
    LED5 = 1 << 4,
    LED6 = 1 << 5,
    LED_ALIVE = LED6,
    LED7 = 1 << 6,
    LED8 = 1 << 7,
    LED9 = 1 << 8,
    LED10 = 1 << 9,
    LED_ALL = 0xFF
};

/*
 * Main task.
 *
 * Blinks LEDs according to ML processing.
 *
 * LED1 on and LED2 off       => heard YES
 * LED1 off and LED2 off      => heard NO
 * LED1 off and LED2 blinking => no/unknown input
 */
void blink_task(void *arg)
{
    (void)arg;

    printf("Blink task started\r\n");

    if (mps3_leds_turn_off(LED_ALL) != true) {
        printf("Failed to turn all LEDs off\r\n");
        return;
    }

    const uint32_t ticks_interval = BLINK_TIMER_PERIOD_MS * osKernelGetTickFreq() / 1000;
    while (1) {
        osStatus_t status = osDelay(ticks_interval);
        if (status != osOK) {
            printf("osDelay() failed: %d\r\n", status);
            return;
        }

        if (mps3_leds_toggle(LED_ALIVE) != true) {
            printf("Failed to toggle LED_ALIVE\r\n");
            return;
        }
    }
}
