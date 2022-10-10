/* Copyright (c) 2021-2022, Arm Limited and Contributors. All rights reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef BLINK_TASK_H
#define BLINK_TASK_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Blinks LEDs according the ML model state.
 */
void blink_task(void *arg);

#ifdef __cplusplus
}
#endif

#endif /* ! BLINK_TASK_H*/
