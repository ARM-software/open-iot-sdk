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

// Events
typedef enum { UI_EVENT_BLINK, UI_EVENT_ML_STATE_CHANGE } ui_state_event_t;

typedef struct {
    ui_state_event_t event;
} ui_msg_t;

static ui_msg_t blink_event = {UI_EVENT_BLINK};

// Message queue
static osMessageQueueId_t ui_msg_queue = NULL;

// Blinking timer
static osTimerId_t blink_timer = NULL;

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

static void blink_timer_cb(void *arg)
{
    (void)arg;
    // Schedule blink of the led in the event queue
    if (osMessageQueuePut(ui_msg_queue, (void *)&blink_event, 0, 0) != osOK) {
        printf("Failed to send blink_event message to ui_msg_queue\r\n");
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

    // Configure the timer
    blink_timer = osTimerNew(blink_timer_cb, osTimerPeriodic, NULL, NULL);
    if (!blink_timer) {
        printf("Create blink timer failed\r\n");
        return;
    }

    // Setup blinking event
    led_off(LED_ALL);

    // start the blinking timer
    uint32_t ticks_interval = ((uint64_t)BLINK_TIMER_PERIOD_MS * (uint64_t)osKernelGetTickFreq()) / 1000;
    osStatus_t res = osTimerStart(blink_timer, ticks_interval);
    if (res) {
        printf("osTimerStart failed %d\r\n", res);
        return;
    }

    while (1) {
        ui_msg_t msg;
        if (osMessageQueueGet(ui_msg_queue, &msg, NULL, osWaitForever) != osOK) {
            continue;
        }

        switch (msg.event) {
            case UI_EVENT_BLINK:
                led_toggle(LED_ALIVE);
                break;

            default:
                break;
        }
    }
}
