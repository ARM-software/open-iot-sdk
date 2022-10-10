/* Copyright (c) 2017-2022, Arm Limited and Contributors. All rights reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "cmsis_os2.h"
#include "iotsdk/ip_network_api.h"

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* includes for TFM */
#include "psa/update.h"

/* includes for IoT Cloud */
#include "aws_dev_mode_key_provisioning.h"
#include "iot_logging_task.h"
#include "ml_interface.h"
#include "ota_provision.h"
#include "version/application_version.h"

#define IOT_LOGGING_QUEUE_SIZE 90

extern void DEMO_RUNNER_RunDemos(void);

psa_key_handle_t xOTACodeVerifyKeyHandle = NULL;

void network_state_callback(network_state_callback_event_t status)
{
    // LwIP may call the callback more than once, e.g. once per IPv4 and IPv6
    static bool handled = false;
    if (handled) {
        return;
    }
    handled = true;

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

int endpoint_init(void)
{
    BaseType_t log_initialized = xLoggingTaskInitialize(0, osPriorityNormal2, IOT_LOGGING_QUEUE_SIZE);
    if (log_initialized != pdTRUE) {
        printf("Failed to initialize logging task [%ld]\r\n", log_initialized);
        return -1;
    }

    CK_RV provisionning_status = vDevModeKeyProvisioning();

    if (provisionning_status == osOK) {
        BaseType_t ret = ota_privision_code_signing_key(&xOTACodeVerifyKeyHandle);
        if (ret != PSA_SUCCESS) {
            printf("ota_privision_code_signing_key failed [%ld]\r\n", ret);
        }
    }

    print_version();

    osStatus_t status = start_network_task(network_state_callback, 0);
    if (status != osOK) {
        printf("Failed to start network task [%d]\r\n", status);
        return -1;
    }

    return 0;
}
