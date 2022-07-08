/*
 * Copyright (c) 2022, Arm Limited and Contributors. All rights reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

/* This example shows device-to-cloud messaging with Azure.
 */

#include <stdlib.h>
#include <inttypes.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>

#include "cmsis_os2.h"
#include "RTOS_config.h"

#include "azure_c_shared_utility/shared_util_options.h"
#include "azure_c_shared_utility/xlogging.h"
#include "iothub_client_options.h"
#include "iothub_device_client_ll.h"
#include "iothub_message.h"
#include "iothub.h"
#include "iothubtransportmqtt.h"

#include "mbedtls/threading.h"
#include "mbedtls/platform.h"

#include "iothub_credentials.h"

#include "ml_interface.h"

/** Azure IoT Hub server TLS certificate - mbedtls specific.
 * It's passed as OPTION_TRUSTED_CERT option to IoT Hub client.
 */
const char *certificates = IOTHUB_SERVER_TLS_CERTIFICATE;
const char *connectionString = IOTHUB_DEVICE_CONNECTION_STRING;

extern void azure_task(void *arg);

int endpoint_init(void)
{
    static const osThreadAttr_t az_task_attr = {.priority = osPriorityNormal, .name = "AZURE_TASK", .stack_size = 8192};
    osThreadId_t demo_thread = osThreadNew(azure_task, NULL, &az_task_attr);

    if (!demo_thread) {
        printf("Failed to create thread\r\n");
        return -1;
    }

    return 0;
}

typedef enum {
    APP_EVENT_IOT_HUB_CONNECTION_UP,
    APP_EVENT_IOT_HUB_CONNECTION_DOWN,
    APP_EVENT_SEND_MSG,
    APP_EVENT_SEND_MSG_OK,
    APP_EVENT_SEND_MSG_ERROR,
    APP_EVENT_NONE
} app_event_t;

typedef struct {
    app_event_t event;
    union {
        int32_t return_code;
        ml_processing_state_t ml_state;
    };
} app_msg_t;

static osMessageQueueId_t app_msg_queue = NULL;

/** This example use NV seed as entropy generator.
 * Set dummy data as NV seed.
 */
static int mbedtls_platform_example_nv_seed_read(unsigned char *buf, size_t buf_len)
{
    if (buf == NULL) {
        return (-1);
    }
    memset(buf, 0xA5, buf_len);
    return 0;
}

static int mbedtls_platform_example_nv_seed_write(unsigned char *buf, size_t buf_len)
{
    return 0;
}

#ifdef IOTSDK_AZURE_SDK_LOGGING
static void iothub_logging(
    LOG_CATEGORY log_category, const char *file, const char *func, int line, unsigned int options, const char *fmt, ...)
{
    const char *p, *basename;

    /* Extract basename from file */
    for (p = basename = file; *p != '\0'; p++) {
        if (*p == '/' || *p == '\\')
            basename = p + 1;
    }

    // determine required buffer size
    va_list args;
    va_start(args, fmt);
    int len = vsnprintf(NULL, 0, fmt, args);
    va_end(args);
    if (len < 0)
        return;

    // format message
    char msg[len + 1];
    va_start(args, fmt);
    vsnprintf(msg, len + 1, fmt, args);
    va_end(args);

    switch (log_category) {
        case AZ_LOG_ERROR:
            printf("Error - %s:%d: %s\r\n", basename, line, msg);
            break;
        case AZ_LOG_INFO:
            printf("Info - %s:%d: %s\r\n", basename, line, msg);
            break;
        case AZ_LOG_TRACE:
            printf("Debug - %s:%d: %s\r\n", basename, line, msg);
            break;
        default:
            break;
    }
}
#endif // IOTSDK_AZURE_SDK_LOGGING

/** This callback is called by the Azure IoT Hub client.
 * It handles IoT Hub connection status.
 */
static void connection_status_callback(IOTHUB_CLIENT_CONNECTION_STATUS result,
                                       IOTHUB_CLIENT_CONNECTION_STATUS_REASON reason,
                                       void *ignored)
{
    ((void)ignored);

    const app_msg_t msg = {.event = (result == IOTHUB_CLIENT_CONNECTION_AUTHENTICATED)
                                        ? APP_EVENT_IOT_HUB_CONNECTION_UP
                                        : APP_EVENT_IOT_HUB_CONNECTION_DOWN,
                           .return_code = (result != IOTHUB_CLIENT_CONNECTION_AUTHENTICATED) ? (int32_t)reason : 0};
    if (osMessageQueuePut(app_msg_queue, &msg, 0, 0) != osOK) {
        printf("Failed to send message to app_msg_queue\r\n");
    }
}

/** This callback is called by the Azure IoT Hub client.
 * It handles confirmation of sending messages to IoT Hub.
 */
static void send_confirm_callback(IOTHUB_CLIENT_CONFIRMATION_RESULT result, void *ignored)
{
    ((void)ignored);

    const app_msg_t msg = {.event = (result == IOTHUB_CLIENT_CONFIRMATION_OK) ? APP_EVENT_SEND_MSG_OK
                                                                              : APP_EVENT_SEND_MSG_ERROR,
                           .return_code = (result != IOTHUB_CLIENT_CONFIRMATION_OK) ? (int32_t)result : 0};
    if (osMessageQueuePut(app_msg_queue, &msg, 0, 0) != osOK) {
        printf("Failed to send message to app_msg_queue\r\n");
    }
}

void mqtt_send_inference_result(ml_processing_state_t state)
{
    // If credentials do not notify the azure task
    if (strcmp(connectionString, "Invalid connection string") == 0) {
        return;
    }

    const app_msg_t msg = {.event = APP_EVENT_SEND_MSG, .ml_state = state};
    if (osMessageQueuePut(app_msg_queue, &msg, 0, 0) != osOK) {
        printf("Failed to send message to app_msg_queue\r\n");
    }
}

void azure_task(void *arg)
{
    // If credentials are not available abandon this thread.
    if (strcmp(connectionString, "Invalid connection string") == 0) {
        printf("Please change IOTHUB_DEVICE_CONNECTION_STRING to setup the cloud connection\r\n");
        ml_task_inference_start();
        osThreadTerminate(osThreadGetId());
        return;
    }

    (void)arg;

    mbedtls_threading_set_cmsis_rtos();
    mbedtls_platform_set_nv_seed(mbedtls_platform_example_nv_seed_read, mbedtls_platform_example_nv_seed_write);

    app_msg_queue = osMessageQueueNew(5, sizeof(app_msg_t), NULL);
    if (!app_msg_queue) {
        printf("Failed to create a app msg queue\r\n");
        return;
    }

    int res = 0;
    IOTHUB_DEVICE_CLIENT_LL_HANDLE iotHubClientHandle = NULL;
    app_msg_t msg;

    printf("Initialising IoT Hub\r\n");

#ifdef IOTSDK_AZURE_SDK_LOGGING
    xlogging_set_log_function(iothub_logging);
#endif // IOTSDK_AZURE_SDK_LOGGING

    res = IoTHub_Init();
    if (res) {
        printf("IoTHub_Init failed: %d\r\n", res);
        goto exit;
    }

    printf("IoT Hub initalization success\r\n");

    printf("IoT hub connection setup\r\n");
    iotHubClientHandle = IoTHubDeviceClient_LL_CreateFromConnectionString(connectionString, MQTT_Protocol);
    if (!iotHubClientHandle) {
        printf("IoTHubDeviceClient_LL_CreateFromConnectionString failed\r\n");
        goto exit;
    }

    IoTHubDeviceClient_LL_SetOption(iotHubClientHandle, OPTION_TRUSTED_CERT, certificates);

    bool traceOn = true;
    IoTHubDeviceClient_LL_SetOption(iotHubClientHandle, OPTION_LOG_TRACE, &traceOn);

    bool urlEncodeOn = true;
    IoTHubDeviceClient_LL_SetOption(iotHubClientHandle, OPTION_AUTO_URL_ENCODE_DECODE, &urlEncodeOn);

    IoTHubDeviceClient_LL_SetConnectionStatusCallback(iotHubClientHandle, connection_status_callback, NULL);

    printf("IoT Hub connecting...\r\n");

    // The function IoTHubDeviceClient_LL_DoWork must be executed every 100ms to work properly.
    // Compute the time we are allowed to wait for a message
    uint32_t msg_timeout_ticks = (100 * osKernelGetTickFreq()) / 1000U;
    msg_timeout_ticks += ((100 * osKernelGetTickFreq()) % 1000U) ? 1 : 0;

    while (1) {
        if (osMessageQueueGet(app_msg_queue, &msg, NULL, msg_timeout_ticks) != osOK) {
            msg.event = APP_EVENT_NONE;
        }

        switch (msg.event) {
            case APP_EVENT_IOT_HUB_CONNECTION_UP:
                printf("IoT Hub connection success\r\n");
                ml_task_inference_start();
                break;

            case APP_EVENT_SEND_MSG: {
                const char *message = get_inference_result_string(msg.ml_state);

                printf("Sending message %s\r\n", message);

                IOTHUB_MESSAGE_HANDLE message_handle = IoTHubMessage_CreateFromString(message);
                if (!message_handle) {
                    printf("IoTHubMessage_CreateFromString failed\r\n");
                    break;
                }

                IOTHUB_CLIENT_RESULT send_res = IoTHubDeviceClient_LL_SendEventAsync(
                    iotHubClientHandle, message_handle, send_confirm_callback, NULL);
                if (send_res != IOTHUB_CLIENT_OK) {
                    printf("Failed to send message %s\r\n", message);
                }
                IoTHubMessage_Destroy(message_handle);

            } break;

            case APP_EVENT_SEND_MSG_OK:
                printf("Ack message\r\n");
                break;
            case APP_EVENT_IOT_HUB_CONNECTION_DOWN:
                printf("IoT Hub connection failed %d\r\n", msg.return_code);
                break;
            case APP_EVENT_SEND_MSG_ERROR:
                printf("Send message failed %d\r\n", msg.return_code);
                break;
            default:
                break;
        }

        IoTHubDeviceClient_LL_DoWork(iotHubClientHandle);
    }

exit:
    printf("Demo stopped\r\n");

    if (iotHubClientHandle) {
        IoTHubDeviceClient_LL_Destroy(iotHubClientHandle);
    }
    IoTHub_Deinit();
    while (1)
        ;
}
