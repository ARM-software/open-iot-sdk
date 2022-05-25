/*
 * Copyright (c) 2022, Arm Limited and Contributors. All rights reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdlib.h>

void *pvPortMalloc(size_t size)
{
    return malloc(size);
}

void vPortFree(void *p)
{
    free(p);
}
