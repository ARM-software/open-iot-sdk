/*
 * Copyright (c) 2022, Arm Limited and Contributors. All rights reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef RTE_COMPONENTS_H
#define RTE_COMPONENTS_H

// TrustZone support
#define DOMAIN_NS 1

// RTOS options
#define OS_DYNAMIC_MEM_SIZE (40000 * 3)
#define OS_ROBIN_ENABLE     1
#define OS_TICK_FREQ        300

#define OS_PRIVILEGE_MODE 1

#endif // RTE_COMPONENTS_H
