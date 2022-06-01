/*
 * Copyright (c) 2022, Arm Limited and Contributors. All rights reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef BASETYPES_H_
#define BASETYPES_H_

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define portCHAR       char
#define portFLOAT      float
#define portDOUBLE     double
#define portLONG       long
#define portSHORT      short
#define portSTACK_TYPE uint32_t
#define portBASE_TYPE  long

typedef portSTACK_TYPE StackType_t;
typedef long BaseType_t;
typedef unsigned long UBaseType_t;

typedef uint32_t TickType_t;

#ifdef __cplusplus
}
#endif

#endif /* BASETYPES_H_ */
