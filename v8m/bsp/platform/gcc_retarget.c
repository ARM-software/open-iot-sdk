/*
 *    Copyright (c) 2006-2023 ARM Limited
 *    All rights reserved.
 *
 *    Licensed under the Apache License, Version 2.0 (the "License");
 *    you may not use this file except in compliance with the License.
 *    You may obtain a copy of the License at
 *
 *        http://www.apache.org/licenses/LICENSE-2.0
 *
 *    Unless required by applicable law or agreed to in writing, software
 *    distributed under the License is distributed on an "AS IS" BASIS,
 *    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *    See the License for the specific language governing permissions and
 *    limitations under the License.
 */

#include <sys/lock.h>
#include <stdalign.h>
#include <stdlib.h>

#if defined(USE_RTX)
    #include "rtx_os.h"
#elif defined(USE_FREERTOS)
    #include "FreeRTOS.h"
    #include "task.h"
    #include "semphr.h"
#endif 

#include "RTOS_config.h"
#include "cmsis_os2.h"

typedef osMutexId_t __lock;

struct __lock { 
    osMutexId_t id;
};

#if defined(USE_RTX)
    #define STATIC_MUTEX_SIZE sizeof(osRtxMutex_t)
    #define STATIC_THREAD_SIZE sizeof(osRtxThread_t)
#elif defined(USE_FREERTOS)
    #define STATIC_MUTEX_SIZE sizeof(StaticSemaphore_t)
    #define STATIC_THREAD_SIZE sizeof(StaticTask_t)
#else
    #define STATIC_MUTEX_SIZE 60
    #define STATIC_THREAD_SIZE 270
#endif 

struct __lock __lock___sinit_recursive_mutex;
struct __lock __lock___sfp_recursive_mutex;
struct __lock __lock___atexit_recursive_mutex;
struct __lock __lock___at_quick_exit_mutex;
struct __lock __lock___malloc_recursive_mutex;
static alignas(8) uint8_t malloc_mutex_obj[STATIC_MUTEX_SIZE];
struct __lock __lock___env_recursive_mutex;
struct __lock __lock___tz_mutex;
struct __lock __lock___dd_hash_mutex;
struct __lock __lock___arc4random_mutex;

void
__retarget_lock_init_static (_LOCK_T lock)
{
    osMutexAttr_t attr = { .attr_bits = osMutexPrioInherit };
    lock->id = osMutexNew(&attr);
}


void
__retarget_lock_init (_LOCK_T *lock)
{
    *lock = malloc(sizeof(struct __lock));
    __retarget_lock_init_static(*lock);
}

void
__retarget_lock_init_recursive_static(_LOCK_T lock)
{
    osMutexAttr_t attr = { .attr_bits = osMutexRecursive | osMutexPrioInherit };
    lock->id = osMutexNew(&attr);
}

void
__retarget_lock_init_recursive(_LOCK_T *lock)
{
    *lock = malloc(sizeof(struct __lock));
    __retarget_lock_init_recursive_static(*lock);
}

void
__retarget_lock_close(_LOCK_T lock)
{
    osMutexRelease(lock->id);
}

void
__retarget_lock_close_recursive(_LOCK_T lock)
{
    osMutexDelete(lock->id);
}

void
__retarget_lock_acquire (_LOCK_T lock)
{
    osMutexAcquire(lock->id, osWaitForever);
}

void
__retarget_lock_acquire_recursive (_LOCK_T lock)
{
    osMutexAcquire(lock->id, osWaitForever);
}

int
__retarget_lock_try_acquire(_LOCK_T lock)
{
    if (osMutexAcquire(lock->id, /* timeout = */ 0) == osOK) { 
        return 0;
    } else { 
        return -1;
    }
}

int
__retarget_lock_try_acquire_recursive(_LOCK_T lock)
{
    if (osMutexAcquire(lock->id, /* timeout = */ 0) == osOK) { 
        return 0;
    } else { 
        return -1;
    }
}

void
__retarget_lock_release (_LOCK_T lock)
{
    osMutexRelease(lock->id);
}

void
__retarget_lock_release_recursive (_LOCK_T lock)
{
    osMutexRelease(lock->id);
}

static void main_thread(void * argument);
static alignas(8) char main_thread_stack[1024 * 4];
static alignas(8) uint8_t main_thread_storage[STATIC_THREAD_SIZE];


// C runtime import: constructor initialization and main
extern void __libc_init_array(void);
extern int main(void);

void software_init_hook(void)
{
    int ret = osKernelInitialize();
    if (ret != osOK)
    {
        abort();
    }

    // Create main thread used to run the application
    {
        osThreadAttr_t main_thread_attr = {
            .name       = "main",
            .cb_mem     = &main_thread_storage,
            .cb_size    = sizeof(main_thread_storage),
            .stack_mem  = main_thread_stack,
            .stack_size = sizeof(main_thread_stack),
            .priority   = osPriorityNormal,
        };

        osThreadId_t main_thread_id = osThreadNew(main_thread, NULL, &main_thread_attr);
        if (main_thread_id == NULL)
        {
            abort();
        }
    }

    ret = osKernelStart();
    // Note osKernelStart should never return
    abort();
}

/**
 * Main thread
 * - Initialize TF-M
 * - Initialize the toolchain:
 *  - Setup mutexes for malloc and environment
 *  - Construct global objects
 * - Run the main
 */
static void main_thread(void * argument)
{
    (void) argument;

    // Initialize malloc mutex
    {
        osMutexAttr_t malloc_mutex_attr = { .name      = "malloc_mutex",
                                            .attr_bits = osMutexRecursive | osMutexPrioInherit,
                                            .cb_mem    = &malloc_mutex_obj,
                                            .cb_size   = sizeof(malloc_mutex_obj) };

        __lock___malloc_recursive_mutex.id = osMutexNew(&malloc_mutex_attr);
        if (__lock___malloc_recursive_mutex.id == NULL)
        {
            abort();
        }
    }

    // Safe to use memory allocation from there

    // Initialize libc locks
    __retarget_lock_init_recursive_static(&__lock___sinit_recursive_mutex);
    __retarget_lock_init_recursive_static(&__lock___sfp_recursive_mutex);
    __retarget_lock_init_recursive_static(&__lock___atexit_recursive_mutex);
    __retarget_lock_init_static(&__lock___at_quick_exit_mutex);
    __retarget_lock_init_recursive_static(&__lock___env_recursive_mutex);
    __retarget_lock_init_static(&__lock___tz_mutex);
    __retarget_lock_init_static(&__lock___dd_hash_mutex);
    __retarget_lock_init_static(&__lock___arc4random_mutex);

    // It is safe to use printf from this point

    /* Run the C++ global object constructors */
    __libc_init_array();

    // It is safe to receive data on serial from this point

    int return_code = main();

    exit(return_code);
}
