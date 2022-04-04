/*
 * Copyright (c) 2021 Arm Limited
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

#include "queue.h"
#include "semphr.h"
#include "task.h"

/****************************************************************************
 * Mutex & Semaphore
 * Overrides weak-linked symbols in ethosu_driver.c to implement thread handling
 ****************************************************************************/

void *ethosu_mutex_create(void)
{
    return xSemaphoreCreateMutex();
}

void ethosu_mutex_lock(void *mutex)
{
    SemaphoreHandle_t handle = (SemaphoreHandle_t)mutex;
    xSemaphoreTake(handle, portMAX_DELAY);
}

void ethosu_mutex_unlock(void *mutex)
{
    SemaphoreHandle_t handle = (SemaphoreHandle_t)mutex;
    xSemaphoreGive(handle);
}

void *ethosu_semaphore_create(void)
{
    return xSemaphoreCreateBinary();
}

void ethosu_semaphore_take(void *sem)
{
    SemaphoreHandle_t handle = (SemaphoreHandle_t)sem;
    xSemaphoreTake(handle, portMAX_DELAY);
}

void ethosu_semaphore_give(void *sem)
{
    SemaphoreHandle_t handle = (SemaphoreHandle_t)sem;
    xSemaphoreGive(handle);
}
