/* Copyright (c) 2021-2022, Arm Limited and Contributors. All rights reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "blink_task.h"

#include "cmsis_os2.h"
#include "ml_interface.h"

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

static uint32_t *fpgaio_leds = (uint32_t *)0x49302000;

// Events
typedef enum { UI_EVENT_ML_STATE_CHANGE } ui_state_event_t;

typedef struct {
    ui_state_event_t event;
} ui_msg_t;

static ui_msg_t ml_state_change_event = {UI_EVENT_ML_STATE_CHANGE};

// Message queue
static osMessageQueueId_t ui_msg_queue = NULL;

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

static void ml_change_handler(void *self, ml_processing_state_t new_state)
{
    (void)self;
    (void)new_state;

    if (osMessageQueuePut(ui_msg_queue, (void *)&ml_state_change_event, 0, 0) != osOK) {
        printf("Failed to send ml_state_change_event message to ui_msg_queue\r\n");
    }
}

void process_ml_state_change(ml_processing_state_t new_state)
{
    switch (new_state) {
        case ML_HEARD_YES:
            printf("ML_HEARD_YES\n");
            led_on(LED_YES);
            break;
        case ML_HEARD_NO:
            printf("ML_HEARD_NO\n");
            led_off(LED_YES);
            break;
        case ML_HEARD_GO:
            printf("ML_HEARD_GO\n");
            led_on(LED_GO);
            break;
        case ML_HEARD_STOP:
            printf("ML_HEARD_GO\n");
            led_off(LED_GO);
            break;
        case ML_HEARD_UP:
            printf("ML_HEARD_GO\n");
            led_on(LED_UP);
            break;
        case ML_HEARD_DOWN:
            printf("ML_HEARD_GO\n");
            led_off(LED_UP);
            break;
        case ML_HEARD_LEFT:
            printf("ML_HEARD_LEFT\n");
            led_on(LED_LEFT);
            break;
        case ML_HEARD_RIGHT:
            printf("ML_HEARD_RIGHT\n");
            led_off(LED_LEFT);
            break;
        case ML_HEARD_ON:
            printf("ML_HEARD_ON\n");
            led_on(LED_ON);
            break;
        case ML_HEARD_OFF:
            printf("ML_HEARD_OFF\n");
            led_off(LED_ON);
            break;
        default:
            printf("ML UNKNOWN\n");
            break;
    }
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
    ui_msg_queue = osMessageQueueNew(10, sizeof(ui_msg_t), NULL);
    if (!ui_msg_queue) {
        printf("Failed to create a ui msg queue\r\n");
        return;
    }

    // Connect to the ML processing
    on_ml_processing_change(ml_change_handler, NULL);

    led_off(LED_ALL);

    // Toggle is-alive LED and process messages at a fixed interval
    const uint32_t ticks_interval = BLINK_TIMER_PERIOD_MS * osKernelGetTickFreq() / 1000;
    while (1) {
        osStatus_t status = osDelay(ticks_interval);
        if (status != osOK) {
            printf("osDelay failed %d\r\n", status);
            return;
        }
        led_toggle(LED_ALIVE);

        // Retrieve any/all existing messages with no delay
        ui_msg_t msg;
        while ((status = osMessageQueueGet(ui_msg_queue, &msg, NULL, 0)) == osOK) {
            switch (msg.event) {
                case UI_EVENT_ML_STATE_CHANGE:
                    process_ml_state_change(get_ml_processing_state());
                    break;
            }
        }
        // The only permitted status at the end is an empty queue
        if (status != osErrorResource) {
            printf("osMessageQueueGet failed: %d\r\n", status);
        }
    }
}
