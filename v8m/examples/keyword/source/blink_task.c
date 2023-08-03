/* Copyright (c) 2021-2023, Arm Limited and Contributors. All rights reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "blink_task.h"
#include "cmsis_os2.h"
#include "ml_interface.h"
#include "mps3_leds.h"
#include <stdbool.h>
#include <stdio.h>

#define BLINK_TIMER_PERIOD_MS 250

enum {
    LED1 = 1 << 0,
    LED_YES = LED1,
    LED2 = 1 << 1,
    LED_GO = LED2,
    LED3 = 1 << 2,
    LED_UP = LED3,
    LED4 = 1 << 3,
    LED_LEFT = LED4,
    LED5 = 1 << 4,
    LED_ON = LED5,
    LED6 = 1 << 5,
    LED_ALIVE = LED6,
    LED7 = 1 << 6,
    LED8 = 1 << 7,
    LED9 = 1 << 8,
    LED10 = 1 << 9,
    LED_ALL = 0xFF
};

// Message queue
static osMessageQueueId_t ui_msg_queue = NULL;

static void ml_change_handler(void *self, ml_processing_state_t new_state)
{
    (void)self;

    if (osMessageQueuePut(ui_msg_queue, &new_state, 0, 0) != osOK) {
        printf("Failed to send ml_state_change_event message to ui_msg_queue\r\n");
    }
}

bool process_ml_state_change(ml_processing_state_t new_state)
{
    if (!serial_lock()) {
        return false;
    }

    switch (new_state) {
        case ML_HEARD_YES:
            printf("ML_HEARD_YES\n");
            if (mps3_leds_turn_on(LED_YES) != true) {
                printf("Failed to turn LED_YES on\r\n");
                return false;
            }
            break;
        case ML_HEARD_NO:
            printf("ML_HEARD_NO\n");
            if (mps3_leds_turn_off(LED_YES) != true) {
                printf("Failed to turn LED_YES off\r\n");
                return false;
            }
            break;
        case ML_HEARD_GO:
            printf("ML_HEARD_GO\n");
            if (mps3_leds_turn_on(LED_GO) != true) {
                printf("Failed to turn LED_GO on\r\n");
                return false;
            }
            break;
        case ML_HEARD_STOP:
            printf("ML_HEARD_STOP\n");
            if (mps3_leds_turn_off(LED_GO) != true) {
                printf("Failed to turn LED_GO off\r\n");
                return false;
            }
            break;
        case ML_HEARD_UP:
            printf("ML_HEARD_UP\n");
            if (mps3_leds_turn_on(LED_UP) != true) {
                printf("Failed to turn LED_UP on\r\n");
                return false;
            }
            break;
        case ML_HEARD_DOWN:
            printf("ML_HEARD_DOWN\n");
            if (mps3_leds_turn_off(LED_UP) != true) {
                printf("Failed to turn LED_UP off\r\n");
                return false;
            }
            break;
        case ML_HEARD_LEFT:
            printf("ML_HEARD_LEFT\n");
            if (mps3_leds_turn_on(LED_LEFT) != true) {
                printf("Failed to turn LED_LEFT on\r\n");
                return false;
            }
            break;
        case ML_HEARD_RIGHT:
            printf("ML_HEARD_RIGHT\n");
            if (mps3_leds_turn_off(LED_LEFT) != true) {
                printf("Failed to turn LED_LEFT off\r\n");
                return false;
            }
            break;
        case ML_HEARD_ON:
            printf("ML_HEARD_ON\n");
            if (mps3_leds_turn_on(LED_ON) != true) {
                printf("Failed to turn LED_ON on\r\n");
                return false;
            }
            break;
        case ML_HEARD_OFF:
            printf("ML_HEARD_OFF\n");
            if (mps3_leds_turn_off(LED_ON) != true) {
                printf("Failed to turn LED_ON off\r\n");
                return false;
            }
            break;
        default:
            printf("ML UNKNOWN\n");
            break;
    }

    serial_unlock();

    return true;
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

    // Create the ui event queue
    ui_msg_queue = osMessageQueueNew(10, sizeof(ml_processing_state_t), NULL);
    if (!ui_msg_queue) {
        printf("Failed to create a ui msg queue\r\n");
        return;
    }

    // Connect to the ML processing
    on_ml_processing_change(ml_change_handler, NULL);

    if (mps3_leds_turn_off(LED_ALL) != true) {
        printf("Failed to turn All LEDs off\r\n");
        return;
    }

    // Toggle is-alive LED and process messages at a fixed interval
    const uint32_t ticks_interval = BLINK_TIMER_PERIOD_MS * osKernelGetTickFreq() / 1000;
    while (1) {
        osStatus_t status = osDelay(ticks_interval);
        if (status != osOK) {
            printf("osDelay failed %d\r\n", status);
            return;
        }
        if (mps3_leds_toggle(LED_ALIVE) != true) {
            printf("Failed to toggle LED_ALIVE\r\n");
            return;
        }

        // Retrieve any/all existing messages with no delay
        ml_processing_state_t msg;
        while ((status = osMessageQueueGet(ui_msg_queue, &msg, NULL, 0)) == osOK) {
            if (process_ml_state_change(msg) != true) {
                printf("Failed to process new ML state\r\n");
                return;
            }
        }
        // The only permitted status at the end is an empty queue
        if (status != osErrorResource) {
            printf("osMessageQueueGet failed: %d\r\n", status);
        }
    }
}
