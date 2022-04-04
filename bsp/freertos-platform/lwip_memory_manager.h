/* Copyright (c) 2021 Arm Limited
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef LWIP_MEMORY_MANAGER_H_
#define LWIP_MEMORY_MANAGER_H_

#include "network_stack_memory_manager.h"

mdh_network_stack_memory_manager_t *lwip_mm_get_instance(void);

#endif // LWIP_MEMORY_MANAGER_H_
