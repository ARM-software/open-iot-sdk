/*
* Copyright (c) 2017 Arm Limited
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

#ifndef __SERIAL_H__
#define __SERIAL_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <stddef.h>

typedef enum serial_cmd_t {
    CMD_YES,
    CMD_NO,
    CMD_GO,
    CMD_STOP
} serial_cmd_t;

typedef void(* serial_cmd_cb_t)(serial_cmd_t);

/**
 * \brief Initializes default UART device
 */
void bsp_serial_init(void);

/**
 * \brief Prints a string through the default UART device
 */
void bsp_serial_print(char *str);

#ifdef __cplusplus
}
#endif

#endif /* __SERIAL_H__ */
