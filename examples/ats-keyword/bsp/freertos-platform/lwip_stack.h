/* Copyright (c) 2021 Arm Limited
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef LWIP_STACK_H_
#define LWIP_STACK_H_

#include "lwip_emac_netif.h"

/* Cause the stack to read the input buffer */
void signal_receive(netif_context_t *context);

/* free rtos task to initialise and run the LWIP stack */
void net_task(void *pvParameters);

#endif // LWIP_STACK_H_
