/* Copyright (c) 2021 Arm Limited
 * SPDX-License-Identifier: Apache-2.0
 */

#include "lwipopts_freertos.h"

#define LWIP_NETIF_LINK_CALLBACK 1
#define MEM_ALIGNMENT            4U

#undef TCPIP_THREAD_PRIO
#define TCPIP_THREAD_PRIO (configMAX_PRIORITIES - 1U)
