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

#include <stdalign.h>
#include <stdlib.h>
#include "cmsis_os2.h"

#if defined(USE_RTX)
    #include "rtx_os.h"
#elif defined(USE_FREERTOS)
    #include "FreeRTOS.h"
    #include "task.h"
#endif 

#if defined(USE_RTX)
    #define STATIC_THREAD_SIZE sizeof(osRtxThread_t)
#elif defined(USE_FREERTOS)
    #define STATIC_THREAD_SIZE sizeof(StaticTask_t)
#else
    #define STATIC_THREAD_SIZE 270
#endif 


static void main_thread(void * argument);
static alignas(8) char main_thread_stack[1024 * 4];
static alignas(8) uint8_t main_thread_storage[STATIC_THREAD_SIZE];

// Initialize the RTOS, start it and then call main into a thread. 
int $Super$$main(void);
int $Sub$$main(void)
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
    return ret;
}

static void main_thread(void * argument)
{
    $Super$$main();
}
