/* Copyright (c) 2021-2022, Arm Limited and Contributors. All rights reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "cmsis_os2.h"

/****************************************************************************
 * Mutex & Semaphore
 * Overrides weak-linked symbols in ethosu_driver.c to implement thread handling
 ****************************************************************************/

void *ethosu_mutex_create(void)
{
    return osMutexNew(NULL);
}

void ethosu_mutex_lock(void *mutex)
{
    osMutexId_t mutex_id = (osMutexId_t)mutex;
    osStatus_t status = osMutexAcquire(mutex_id, osWaitForever);
    if (status != osOK) {
        printf("osMutexAcquire failed %d\r\n", status);
    }
}

void ethosu_mutex_unlock(void *mutex)
{
    osMutexId_t mutex_id = (osMutexId_t)mutex;
    osStatus_t status = osMutexRelease(mutex_id);
    if (status != osOK) {
        printf("osMutexRelease failed %d\r\n", status);
    }
}

void *ethosu_semaphore_create(void)
{
    return osSemaphoreNew(1U, 1U, NULL);
}

void ethosu_semaphore_take(void *sem)
{
    osSemaphoreId_t sem_id = (osSemaphoreId_t)sem;
    osStatus_t status = osSemaphoreAcquire(sem_id, osWaitForever);
    if (status != osOK) {
        printf("osSemaphoreAcquire failed %d\r\n", status);
    }
}

void ethosu_semaphore_give(void *sem)
{
    osSemaphoreId_t sem_id = (osSemaphoreId_t)sem;
    osStatus_t status = osSemaphoreRelease(sem_id);
    if (status != osOK) {
        printf("osSemaphoreRelease failed %d\r\n", status);
    }
}
