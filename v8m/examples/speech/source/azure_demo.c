/*
 * Copyright (c) 2022, Arm Limited and Contributors. All rights reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

/* This example shows device-to-cloud messaging with Azure.
 */

#include "RTOS_config.h"
#include "azure_c_shared_utility/shared_util_options.h"
#include "azure_c_shared_utility/xlogging.h"
#include "azure_prov_client/iothub_security_factory.h"
#include "azure_prov_client/prov_device_client.h"
#include "azure_prov_client/prov_security_factory.h"
#include "azure_prov_client/prov_transport_mqtt_client.h"
#include "cmsis_os2.h"
#include "dsp_task.h"
#include "iothub.h"
#include "iothub_client_options.h"
#include "iothub_credentials.h"
#include "iothub_device_client_ll.h"
#include "iothub_message.h"
#include "iothubtransportmqtt.h"
#include "mbedtls/platform.h"
#include "mbedtls/threading.h"
#include "ml_interface.h"

#include <inttypes.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define VERIFY_OR_EXIT(cond, string, ...)     \
    if (!(cond)) {                            \
        printf(string "\r\n", ##__VA_ARGS__); \
        goto exit;                            \
    }

/* how many times to retry connecting before giving up */
#define AZURE_HUB_RETRIES 10

/** Azure IoT Hub server TLS certificate - mbedtls specific.
 * It's passed as OPTION_TRUSTED_CERT option to IoT Hub client.
 */
static const char *certificates = IOTHUB_SERVER_TLS_CERTIFICATE;

/* provisioning config */
static const char *dpsEndpointString = IOTHUB_DPS_ENDPOINT;
static const char *dpsScopeString = IOTHUB_DPS_ID_SCOPE;
static const char *dpsRegistrationString = IOTHUB_DPS_REGISTRATION_ID;
static const char *dpsKeyString = IOTHUB_DPS_KEY;
/* after provisioning, this will store the result endpoint URI and device id */
static char *iotHubUri = NULL;
static char *deviceId = NULL;

/* direct connection config (alternative to provisioning) */
static char *connectionString = IOTHUB_DEVICE_CONNECTION_STRING;

/* optional model string */
static const char *modelString = IOTHUB_MODEL_STRING;

static osTimerId_t telemetryTimer = NULL;

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
    APP_EVENT_IOT_HUB_START_COMMISSIONING,
    APP_EVENT_IOT_HUB_COMMISSIONED,
    APP_EVENT_SEND_TELEMETRY,
    APP_EVENT_SEND_MSG,
    APP_EVENT_SEND_MSG_OK,
    APP_EVENT_SEND_MSG_ERROR,
    APP_EVENT_SEND_MSG_MEMORY_ALOCATION_ERROR,
    APP_EVENT_NONE
} app_event_t;

typedef struct {
    app_event_t event;
    union {
        int32_t return_code;
        const char *ml_message;
    };
} app_msg_t;

static osMessageQueueId_t app_msg_queue = NULL;

static void send_app_msg(app_event_t event, int32_t code)
{
    const app_msg_t msg = {.event = event, .return_code = code};
    if (osMessageQueuePut(app_msg_queue, &msg, 0, 0) != osOK) {
        printf("Failed to send message to app_msg_queue\r\n");
    }
}

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
    (void)buf;
    (void)buf_len;
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
static void connection_status_cb(IOTHUB_CLIENT_CONNECTION_STATUS result,
                                 IOTHUB_CLIENT_CONNECTION_STATUS_REASON reason,
                                 void *ignored)
{
    (void)ignored;
    if (result == IOTHUB_CLIENT_CONNECTION_AUTHENTICATED) {
        send_app_msg(APP_EVENT_IOT_HUB_CONNECTION_UP, 0);
    } else {
        send_app_msg(APP_EVENT_IOT_HUB_CONNECTION_DOWN, (int32_t)reason);
    }
}

/** This callback is called by the Azure IoT Hub client.
 * It handles confirmation of sending messages to IoT Hub.
 */
static void send_confirm_cb(IOTHUB_CLIENT_CONFIRMATION_RESULT result, void *ignored)
{
    (void)ignored;
    if (result == IOTHUB_CLIENT_CONFIRMATION_OK) {
        send_app_msg(APP_EVENT_SEND_MSG_OK, 0);
    } else {
        send_app_msg(APP_EVENT_SEND_MSG_ERROR, (int32_t)result);
    }
}

static void provisioning_register_cb(PROV_DEVICE_RESULT register_result,
                                     const char *iothub_uri,
                                     const char *device_id,
                                     void *user_context)
{
    (void)user_context;

    if (register_result != PROV_DEVICE_RESULT_OK) {
        printf("Provisioning callback called with error state %d\r\n", register_result);
        send_app_msg(APP_EVENT_IOT_HUB_COMMISSIONED, (int32_t)register_result);
    } else {
        iotHubUri = (char *)malloc(strlen(iothub_uri) + 1);
        deviceId = (char *)malloc(strlen(device_id) + 1);
        if (!iotHubUri || !deviceId) {
            send_app_msg(APP_EVENT_SEND_MSG_MEMORY_ALOCATION_ERROR, 0);
        } else {
            memcpy(iotHubUri, iothub_uri, strlen(iothub_uri) + 1);
            memcpy(deviceId, device_id, strlen(device_id) + 1);
            send_app_msg(APP_EVENT_IOT_HUB_COMMISSIONED, 0);
        }
    }
}

static void telemetry_timer_cb()
{
    send_app_msg(APP_EVENT_SEND_TELEMETRY, 0);
}

void mqtt_send_inference_result(const char *message)
{
    if (!message) {
        return;
    }

    if (!app_msg_queue) {
        return;
    }

    // Copy the string into its own memory buffer
    size_t msg_len = strlen(message) + 1;
    char *msg_copy = malloc(msg_len);
    memcpy(msg_copy, message, msg_len);

    const app_msg_t msg = {.event = APP_EVENT_SEND_MSG, .ml_message = msg_copy};
    if (osMessageQueuePut(app_msg_queue, &msg, 0, 0) != osOK) {
        printf("Failed to send message to app_msg_queue\r\n");
        free(msg_copy);
    }
}

void azure_task(void *arg)
{
    (void)arg;

    int res = 0;
    app_msg_t msg;
    IOTHUB_DEVICE_CLIENT_LL_HANDLE client_handle = NULL;
    PROV_DEVICE_LL_HANDLE prov_handle = NULL;
    size_t retries = AZURE_HUB_RETRIES;
    bool iotHubConnected = false;

    bool valid_connection_string = strcmp(connectionString, "Invalid connection string") != 0;
    bool valid_provisioning_config = (strlen(dpsEndpointString) > 0) && (strlen(dpsScopeString) > 0)
                                     && (strlen(dpsRegistrationString) > 0) && (strlen(dpsKeyString) > 0);

    // either the connection string or provisioning config must be set
    if (!valid_connection_string && !valid_provisioning_config) {
        printf("Please change IOTHUB_DEVICE_CONNECTION_STRING or add provisioning configuration"
               "to setup the cloud connection\r\n");
        ml_task_inference_start();
        dsp_task_start();
        osThreadTerminate(osThreadGetId());
        return;
    }

    mbedtls_threading_set_cmsis_rtos();
    mbedtls_platform_set_nv_seed(mbedtls_platform_example_nv_seed_read, mbedtls_platform_example_nv_seed_write);

    app_msg_queue = osMessageQueueNew(5, sizeof(app_msg_t), NULL);
    VERIFY_OR_EXIT(app_msg_queue, "Failed to create a app msg queue");

    // The timer is restarted manually when a telemetry message has been sent
    telemetryTimer = osTimerNew(telemetry_timer_cb, osTimerOnce, NULL, NULL);
    VERIFY_OR_EXIT(telemetryTimer, "Creating telemetryTimer failed (osTimerNew returned NULL)");

    printf("Initialising IoT Hub\r\n");

#ifdef IOTSDK_AZURE_SDK_LOGGING
    xlogging_set_log_function(iothub_logging);
#endif // IOTSDK_AZURE_SDK_LOGGING

    res = IoTHub_Init();
    VERIFY_OR_EXIT(!res, "IoTHub_Init failed: %d", res);

    printf("IoT Hub initalization success\r\n");

    // we skip APP_EVENT_IOT_HUB_START_COMMISSIONING if we already have a valid connection string
    send_app_msg(valid_connection_string ? APP_EVENT_IOT_HUB_COMMISSIONED : APP_EVENT_IOT_HUB_START_COMMISSIONING, 0);

    const uint32_t tick_freq = osKernelGetTickFreq();
    // The do work functions must be executed at least every 100ms to work properly.
    // Compute the time we are allowed to wait for a message, given a target of 20ms
    uint32_t msg_timeout_ticks = (20 * tick_freq) / 1000U;

    if (tick_freq < 50) {
        printf("Kernel tick length is longer than 20ms, this might cause issues with thread dispatching\r\n");
        msg_timeout_ticks += 1; // wait at least one tick
    }

    while (1) {
        if (client_handle) {
            IoTHubDeviceClient_LL_DoWork(client_handle);
        }
        if (prov_handle) {
            Prov_Device_LL_DoWork(prov_handle);
        }

        if (osMessageQueueGet(app_msg_queue, &msg, NULL, msg_timeout_ticks) != osOK) {
            msg.event = APP_EVENT_NONE;
        }

        if (!serial_lock()) {
            return;
        }
        switch (msg.event) {
            case APP_EVENT_IOT_HUB_START_COMMISSIONING: {
                printf("Setting up provisioning for device %s with endpoint %s and scope id %s\r\n",
                       dpsRegistrationString,
                       dpsEndpointString,
                       dpsScopeString);

                res = prov_dev_set_symmetric_key_info(dpsRegistrationString, dpsKeyString);
                VERIFY_OR_EXIT(!res, "prov_dev_set_symmetric_key_info failed: %d", res);

                res = prov_dev_security_init(SECURE_DEVICE_TYPE_SYMMETRIC_KEY);
                VERIFY_OR_EXIT(!res, "prov_dev_security_init failed: %d", res);

                prov_handle = Prov_Device_LL_Create(dpsEndpointString, dpsScopeString, Prov_Device_MQTT_Protocol);
                VERIFY_OR_EXIT(prov_handle, "Failed to create Provisioning Client");

                res = Prov_Device_LL_Register_Device(prov_handle, provisioning_register_cb, NULL, NULL, NULL);
                VERIFY_OR_EXIT(!res, "Prov_Device_LL_Register_Device failed: %d", res);

                res = Prov_Device_LL_SetOption(prov_handle, OPTION_TRUSTED_CERT, certificates);
                VERIFY_OR_EXIT(!res, "Prov_Device_LL_SetOption OPTION_TRUSTED_CERT failed: %d", res);

                bool option_on = true;
                res = Prov_Device_LL_SetOption(prov_handle, OPTION_LOG_TRACE, &option_on);
                VERIFY_OR_EXIT(!res, "Prov_Device_LL_SetOption OPTION_LOG_TRACE failed: %d", res);
            } break;
            case APP_EVENT_IOT_HUB_COMMISSIONED: {
                if (msg.return_code != 0) {
                    if (retries--) {
                        printf("Provisioning failed, retrying\r\n");
                        send_app_msg(APP_EVENT_IOT_HUB_START_COMMISSIONING, 0);
                        break;
                    } else {
                        printf("Provisioning failed, giving up connecting to Azure cloud\r\n");
                        dsp_task_start();
                        ml_task_inference_start();
                        osThreadTerminate(osThreadGetId());
                    }
                }

                if (prov_handle) {
                    printf("Successfully provisioned\r\n");

                    Prov_Device_LL_Destroy(prov_handle);
                    prov_handle = NULL;

                    res = iothub_security_init(IOTHUB_SECURITY_TYPE_SYMMETRIC_KEY);
                    VERIFY_OR_EXIT(!res, "iothub_security_init failed with code: %d", res);

                    printf("Connecting to %s with device id %s\r\n", iotHubUri, deviceId);

                    client_handle = IoTHubDeviceClient_LL_CreateFromDeviceAuth(iotHubUri, deviceId, MQTT_Protocol);
                    VERIFY_OR_EXIT(client_handle, "IoTHubDeviceClient_LL_CreateFromDeviceAuth failed");
                } else {
                    printf("IoT Hub connection based on user provided connection string\r\n");

                    client_handle = IoTHubDeviceClient_LL_CreateFromConnectionString(connectionString, MQTT_Protocol);
                    VERIFY_OR_EXIT(client_handle, "IoTHubDeviceClient_LL_CreateFromConnectionString failed");
                }

                if (strlen(modelString) > 0) {
                    res = IoTHubDeviceClient_LL_SetOption(client_handle, OPTION_MODEL_ID, modelString);
                    VERIFY_OR_EXIT(!res, "IoTHubDeviceClient_LL_SetOption TRUSTED_CERT failed with code: %d", res);
                }

                res = IoTHubDeviceClient_LL_SetOption(client_handle, OPTION_TRUSTED_CERT, certificates);
                VERIFY_OR_EXIT(!res, "IoTHubDeviceClient_LL_SetOption TRUSTED_CERT failed with code: %d", res);

                bool option_on = true;
                res = IoTHubDeviceClient_LL_SetOption(client_handle, OPTION_LOG_TRACE, &option_on);
                VERIFY_OR_EXIT(!res, "IoTHubDeviceClient_LL_SetOption LOG_TRACE failed with code: %d", res);

                res = IoTHubDeviceClient_LL_SetOption(client_handle, OPTION_AUTO_URL_ENCODE_DECODE, &option_on);
                VERIFY_OR_EXIT(
                    !res, "IoTHubDeviceClient_LL_SetOption AUTO_URL_ENCODE_DECODE failed with code: %d", res);

                IoTHubDeviceClient_LL_SetConnectionStatusCallback(client_handle, connection_status_cb, NULL);

                printf("IoT Hub connecting...\r\n");
            } break;
            case APP_EVENT_IOT_HUB_CONNECTION_UP: {
                iotHubConnected = true;
                printf("IoT Hub connection success\r\n");
                dsp_task_start();
                ml_task_inference_start();
                // start sending telemetry
                res = osTimerStart(telemetryTimer, osKernelGetTickFreq());
                VERIFY_OR_EXIT(!res, "osTimerStart telemetryTimer failed with code: %d", res);
            } break;
            case APP_EVENT_SEND_MSG: {
                if (!iotHubConnected) {
                    break;
                }
                const char *message = msg.ml_message;

                printf("Sending message %s\r\n", message);

                IOTHUB_MESSAGE_HANDLE message_handle = IoTHubMessage_CreateFromString(message);
                if (!message_handle) {
                    printf("IoTHubMessage_CreateFromString failed\r\n");
                    free((void *)message);
                    goto exit;
                }

                res = IoTHubDeviceClient_LL_SendEventAsync(client_handle, message_handle, send_confirm_cb, NULL);
                if (res != IOTHUB_CLIENT_OK) {
                    printf("Failed to send message %s\r\n", message);
                }
                IoTHubMessage_Destroy(message_handle);
                free((void *)message);

            } break;
            case APP_EVENT_SEND_TELEMETRY: {
                if (iotHubConnected) {
                    printf("Sending telemetry\r\n");
                    IOTHUB_MESSAGE_HANDLE message_handle = IoTHubMessage_CreateFromString("test");
                    VERIFY_OR_EXIT(message_handle, "IoTHubMessage_CreateFromString failed.");
                    res = IoTHubMessage_SetProperty(message_handle, "property_key", "property_value");
                    VERIFY_OR_EXIT(!res, "IoTHubMessage_SetProperty failed: %d", res);
                    res = IoTHubDeviceClient_LL_SendEventAsync(client_handle, message_handle, send_confirm_cb, NULL);
                    VERIFY_OR_EXIT(!res, "IoTHubDeviceClient_LL_SendEventAsync failed: %d", res);
                    IoTHubMessage_Destroy(message_handle);
                }
                // restart sending telemetry
                res = osTimerStart(telemetryTimer, osKernelGetTickFreq());
                VERIFY_OR_EXIT(!res, "osTimerStart telemetryTimer failed with code: %d", res);
            } break;
            case APP_EVENT_SEND_MSG_OK:
                printf("Message sent\r\n");
                break;
            case APP_EVENT_IOT_HUB_CONNECTION_DOWN:
                iotHubConnected = false;
                printf("IoT Hub connection failed %d\r\n", msg.return_code);
                break;
            case APP_EVENT_SEND_MSG_ERROR:
                printf("Send message failed %d\r\n", msg.return_code);
                break;
            case APP_EVENT_NONE:
                break;
            case APP_EVENT_SEND_MSG_MEMORY_ALOCATION_ERROR:
                printf("Azure demo memory alocation error\r\n");
                goto exit;
            default:
                break;
        }
        serial_unlock();
    }

exit:
    printf("Demo stopped\r\n");

    if (client_handle) {
        IoTHubDeviceClient_LL_Destroy(client_handle);
    }
    if (prov_handle) {
        Prov_Device_LL_Destroy(prov_handle);
    }
    free(iotHubUri);
    free(deviceId);
    osTimerDelete(telemetryTimer); // safe to call with NULL
    IoTHub_Deinit();
    while (1)
        ;
}
