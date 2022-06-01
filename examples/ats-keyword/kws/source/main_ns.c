/* Copyright (c) 2017-2022, Arm Limited and Contributors. All rights reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include "cmsis_os2.h"
#include "bsp_serial.h"

#include "blink_task.h"
#include "ml_interface.h"

#include "iotsdk/ip_network_api.h"

/* includes for TFM */
#include "psa/update.h"

/* includes for IoT Cloud */
#include "iot_logging_task.h"
#include "aws_dev_mode_key_provisioning.h"
#include "ota_provision.h"
#include "version/application_version.h"

#define IOT_LOGGING_QUEUE_SIZE 90

extern int mbedtls_platform_set_calloc_free(void *(*calloc_func)(size_t, size_t), void (*free_func)(void *));
static void *prvCalloc(size_t xNmemb, size_t xSize);

extern void DEMO_RUNNER_RunDemos(void);

/*
 * Semihosting is a mechanism that enables code running on an ARM target
 * to communicate and use the Input/Output facilities of a host computer
 * that is running a debugger.
 * There is an issue where if you use armclang at -O0 optimisation with
 * no parameters specified in the main function, the initialisation code
 * contains a breakpoint for semihosting by default. This will stop the
 * code from running before main is reached.
 * Semihosting can be disabled by defining __ARM_use_no_argv symbol
 * (or using higher optimization level).
 */
#if defined(__ARMCC_VERSION) && (__ARMCC_VERSION >= 6010050)
__asm("  .global __ARM_use_no_argv\n");
#endif

extern uint32_t tfm_ns_interface_init(void);

psa_key_handle_t xOTACodeVerifyKeyHandle = NULL;

void network_state_callback(network_state_callback_event_t status)
{
    if (status == NETWORK_UP) {
        printf("[INF] network up, starting demo\r\n");

        if (strcmp(clientcredentialMQTT_BROKER_ENDPOINT, "endpointid.amazonaws.com") == 0) {
            printf("[ERR] INVALID CREDENTIALS AND ENDPOINT.\r\n");
            printf("[ERR] Set the right configuration and credentials in aws_clientcredential.h and "
                   "aws_clientcredential_keys.h\r\n");
            // Start the inference directly
            ml_task_inference_start();
        } else {
            DEMO_RUNNER_RunDemos();
        }
    } else {
        printf("[ERR] network down\r\n");
    }
}

void print_version()
{
    if (GetImageVersionPSA(FWU_IMAGE_TYPE_NONSECURE) == 0) {
        printf("Firmware version: %d.%d.%d\r\n",
               xAppFirmwareVersion.u.x.major,
               xAppFirmwareVersion.u.x.minor,
               xAppFirmwareVersion.u.x.build);
    }
}

extern void vUARTLockInit(void);

void main_task(void *arg)
{
    tfm_ns_interface_init();

    vUARTLockInit();

    static const osThreadAttr_t ml_task_attr = {.priority = osPriorityNormal1, .stack_size = 8192};
    osThreadId_t ml_thread = osThreadNew(ml_task, NULL, &ml_task_attr);
    if (!ml_thread) {
        printf("Failed to create ml thread\r\n");
        return;
    }

    static const osThreadAttr_t blink_attr = {.priority = osPriorityHigh};
    osThreadId_t blink_thread = osThreadNew(blink_task, NULL, &blink_attr);
    if (!blink_thread) {
        printf("Failed to create blink thread\r\n");
        return;
    }

    osThreadId_t ml_mqtt_thread = osThreadNew(ml_mqtt_task, NULL, NULL);
    if (!ml_mqtt_thread) {
        printf("Failed to create ml mqtt thread\r\n");
        return;
    }

    BaseType_t log_initialized = xLoggingTaskInitialize(0, osPriorityNormal2, IOT_LOGGING_QUEUE_SIZE);
    if (log_initialized != pdTRUE) {
        printf("Failed to initialize logging task [%ld]\r\n", log_initialized);
        return;
    }

    mbedtls_platform_set_calloc_free(prvCalloc, free);
    CK_RV provisionning_status = vDevModeKeyProvisioning();

    if (provisionning_status == osOK) {
        BaseType_t ret = ota_privision_code_signing_key(&xOTACodeVerifyKeyHandle);
        if (ret != PSA_SUCCESS) {
            printf("ota_privision_code_signing_key failed [%d]\r\n", ret);
        }
    }

    print_version();

    osStatus_t status = start_network_task(network_state_callback, 0);
    if (status != osOK) {
        printf("Failed to start network task [%d]\r\n", status);
    }

    while (1) {
        osDelay(osWaitForever);
    };
    return;
}

int main()
{
    bsp_serial_init();

    osStatus_t os_status = osKernelInitialize();
    if (os_status != osOK) {
        printf("osKernelInitialize failed: %d\r\n", os_status);
        return EXIT_FAILURE;
    }

    osThreadId_t connectivity_thread = osThreadNew(main_task, NULL, NULL);
    if (!connectivity_thread) {
        printf("Failed to create connectivity thread\r\n");
        return EXIT_FAILURE;
    }

    osKernelState_t os_state = osKernelGetState();
    if (os_state != osKernelReady) {
        printf("Kernel not ready %d\r\n", os_state);
        return EXIT_FAILURE;
    }

    printf("Starting scheduler from ns main\r\n");

    /* Start the scheduler itself. */
    os_status = osKernelStart();
    if (os_status != osOK) {
        printf("Failed to start kernel: %d\r\n", os_status);
        return EXIT_FAILURE;
    }

    return 0;
}

static void *prvCalloc(size_t xNmemb, size_t xSize)
{
    void *pvNew = pvPortMalloc(xNmemb * xSize);

    if (NULL != pvNew) {
        memset(pvNew, 0, xNmemb * xSize);
    }

    return pvNew;
}
