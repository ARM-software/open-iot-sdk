/*
* Copyright (c) 2017-2022 Arm Limited
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
#include <string.h>

#include "bsp_serial.h"
#include "hal/serial_api.h"
#include "mps3_uart.h"

static mps3_uart_t *my_uart;

void bsp_serial_init(void)
{
    mps3_uart_init(&my_uart, &UART0_CMSDK_DEV_NS);
    mdh_serial_set_baud(&my_uart->serial, 115200);
}

void bsp_serial_print(char *str)
{
    while (*str != '\0') {
        mdh_serial_put_data(&my_uart->serial, *str++);
    }
}

/* Redirects gcc printf to UART0 */
int _write(int fd, char *str, int len)
{
    int cnt = len;
    while(cnt > 0) {
        mdh_serial_put_data(&my_uart->serial, *str++);
        --cnt;
    }

    return len - cnt;
}
