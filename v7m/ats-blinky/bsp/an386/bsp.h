/*
 * Copyright (c) 2022, Arm Limited and Contributors. All rights reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef IOTSDK_BLINKY_BSP_AN552
#define IOTSDK_BLINKY_BSP_AN552

#include "hal/gpio_api.h"
#include "hal/serial_api.h"

mdh_serial_t *bsp_serial_init(void);
mdh_gpio_t *bsp_gpio_init(void);

#endif
