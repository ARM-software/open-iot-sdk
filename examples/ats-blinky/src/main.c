/*
 * Copyright (c) 2022, Arm Limited and Contributors. All rights reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <inttypes.h>
#include <stddef.h>

#include "bsp.h"
#include "hal/serial_api.h"

#include "FreeRTOS.h"
#include "semphr.h"
#include "task.h"

static SemaphoreHandle_t gs_serial_mutex = NULL;
static mdh_serial_t *gs_serial_output = NULL;

void serial_print(const char *message)
{
    if ((NULL == gs_serial_mutex) || (NULL == gs_serial_output) || (NULL == message)) {
        return;
    }

    if (xSemaphoreTake(gs_serial_mutex, portMAX_DELAY) != pdTRUE) {
        return;
    }

    while (*message != 0) {
        while (!mdh_serial_is_writable(gs_serial_output)) {
        }
        mdh_serial_put_data(gs_serial_output, *message);
        message += 1;
    }
    xSemaphoreGive(gs_serial_mutex);
}

/**
 * Writes to the global serial gs_serial_output the string received as argument.
 *
 * @param argument  A nul terminated string.
 */
static void print_task(void *argument)
{
    const TickType_t xDelay = 500 / portTICK_PERIOD_MS;
    TickType_t now = xTaskGetTickCount();

    for (int i = 0; i < 5; ++i) {
        serial_print((const char *)argument);
        xTaskDelayUntil(&now, xDelay);
    }
    vTaskDelete(NULL);
}

/**
 * Blinks the LED received as argument.
 *
 * @param argument A pointer to a mdh_gpio_t to be toggled on and off.
 */
static void blinky_task(void *argument)
{
    mdh_gpio_t *led = (mdh_gpio_t *)argument;

    const TickType_t xDelay = 1000 / portTICK_PERIOD_MS;
    TickType_t now = xTaskGetTickCount();

    while (true) {
        mdh_gpio_write(led, 1 - mdh_gpio_read(led));
        xTaskDelayUntil(&now, xDelay);
    }
}

int main()
{
    gs_serial_mutex = xSemaphoreCreateMutex();
    gs_serial_output = bsp_serial_init();
    mdh_gpio_t *led = bsp_gpio_init();
    serial_print("Inside main()\r\n");

    TaskHandle_t task_id_a;
    TaskHandle_t task_id_b;
    TaskHandle_t task_blinky;

    xTaskCreate(print_task,
                "TaskA",
                configMINIMAL_STACK_SIZE * 2,
                "Message from Thread A\r\n",
                configMAX_PRIORITIES - 2,
                &task_id_a);
    xTaskCreate(print_task,
                "TaskB",
                configMINIMAL_STACK_SIZE * 2,
                "Message from Thread B\r\n",
                configMAX_PRIORITIES - 2,
                &task_id_b);
    xTaskCreate(blinky_task, "Blinky", configMINIMAL_STACK_SIZE * 2, led, configMAX_PRIORITIES - 2, &task_blinky);
    serial_print("Tasks created\r\n");

    vTaskStartScheduler();
    serial_print("end\r\n");
    return 0;
};
