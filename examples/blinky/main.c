/* Copyright (c) 2017-2023, Arm Limited and Contributors. All rights reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "cmsis_os2.h"
#include "mps3_leds.h"
#include "tfm_ns_interface.h"

#include <stdio.h>
#include <stdlib.h>

#define LEDS_STATE_LED_0_ON     (0x00000001U)
#define LEDS_STATE_ALL_LEDS_OFF (0x00000000U)
#define LED_STATE_CHANGE_DELAY  1000U // 1 Second

extern uint32_t tfm_ns_interface_init(void);

int main()
{
    mps3_leds_init();

    printf("The LED started blinking...\r\n");
    while (1) {
        if (mps3_leds_set_state(LEDS_STATE_LED_0_ON) != true) {
            printf("Turning LED 0 on failed\r\n");
            return -1;
        }
        printf("LED on\r\n");
        osDelay(LED_STATE_CHANGE_DELAY);

        if (mps3_leds_set_state(LEDS_STATE_ALL_LEDS_OFF) != true) {
            printf("Turning LEDs off failed\r\n");
            return -1;
        }
        printf("LED off\r\n");
        osDelay(LED_STATE_CHANGE_DELAY);
    }
    return 0;
}
