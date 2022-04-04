/*
 * Copyright (c) 2017-2021 Arm Limited
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <stdio.h>
#include "FreeRTOS.h"
#include "task.h"
#include "tfm_ns_interface.h"
#include "psa/protected_storage.h"
#include "bsp_serial.h"

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

uint8_t ucHeap[configTOTAL_HEAP_SIZE];

/*
 * Main task to run TFM and ethernet communication testing
 */
static void blink_task(void *pvParameters)
{
    uint32_t *fpgaio_leds = (uint32_t *)0x49302000;
    const TickType_t xDelay = portTICK_PERIOD_MS * 200;

    printf("FreeRTOS blink task started\r\n");
    while (1) {
        *fpgaio_leds = 0xFF;
        printf("LED on\r\n");
        vTaskDelay(xDelay);
        *fpgaio_leds = 0x00;
        printf("LED off\r\n");
        vTaskDelay(xDelay);
    }
}

int main()
{
    tfm_ns_interface_init();
    bsp_serial_init();

    xTaskCreate(blink_task, "test task", configMINIMAL_STACK_SIZE * 2, NULL, configMAX_PRIORITIES - 2, NULL);

    /* Start the scheduler itself. */
    vTaskStartScheduler();

    printf("End of main. Halting!\r\n");
    while (1) {
    }
}

/**
 * @brief   Defines the Ethos-U interrupt handler: just a wrapper around the default
 *          implementation.
 **/
void arm_npu_irq_handler(void)
{
}
