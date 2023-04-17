/* Copyright (c) 2021-2022, Arm Limited and Contributors. All rights reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "blink_task.h"

#include "cmsis_os2.h"

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

static uint32_t *fpgaio_leds = (uint32_t *)0x49302000;

void led_on(uint8_t bits)
{
    *fpgaio_leds |= bits;
}

void led_off(uint8_t bits)
{
    *fpgaio_leds &= ~bits;
}

void led_toggle(uint8_t bits)
{
    *fpgaio_leds ^= bits;
}

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

    led_off(LED_ALL);

    const uint32_t ticks_interval = BLINK_TIMER_PERIOD_MS * osKernelGetTickFreq() / 1000;
    while (1) {
        osStatus_t status = osDelay(ticks_interval);
        if (status != osOK) {
            printf("osDelay() failed: %d\r\n", status);
            return;
        }

        led_toggle(LED_ALIVE);
    }
}
