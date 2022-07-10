/*
 * Copyright (c) 2022, Arm Limited and Contributors. All rights reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "bsp.h"
#include "mps3_uart.h"
#include "mps3_io.h"

mdh_serial_t *bsp_serial_init(void)
{
    mps3_uart_t *uart = NULL;
    mps3_uart_init(&uart, &UART0_CMSDK_DEV);
    return &uart->serial;
}

mdh_gpio_t *bsp_gpio_init(void)
{
    static mps3_io_t led;
    mps3_io_init(&led, &MPS3_IO_DEV_NS, USERLED1);

    mdh_gpio_set_direction(&(led.gpio), MDH_GPIO_DIRECTION_OUT);
    return &led.gpio;
}
