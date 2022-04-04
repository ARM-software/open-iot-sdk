/*
 * Copyright (c) 2022, Arm Limited and Contributors. All rights reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "bsp.h"
#include "mps3_uart.h"

mdh_serial_t *bsp_serial_init(void)
{
    mps3_uart_t *uart = NULL;
    mps3_uart_init(&uart, &UART0_CMSDK_DEV);
    return &uart->serial;
}

gpio_t *bsp_gpio_init(void)
{
    static gpio_t led = {0};
    gpio_init_out(&led, USERLED1);
    return &led;
}
