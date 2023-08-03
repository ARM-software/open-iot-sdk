/* Copyright (c) 2022-2023, Arm Limited and Contributors. All rights reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "iotsdk/nx_driver_cmsis_eth.h"
#include "azure/core/az_log.h"
#include "azure/iot/az_iot_common.h"
#include "azure_iot_credentials.h"
#include "azure_netx_config.h"
#include "cmsis_os2.h"
#include "cmsis_os2_tx_extensions.h"
#include "ml_interface.h"
#include "nx_api.h"
#include "nx_azure_iot.h"
#include "nx_azure_iot_adu_agent.h"
#include "nx_azure_iot_adu_agent_psa_driver.h"
#include "nx_azure_iot_cert.h"
#include "nx_azure_iot_ciphersuites.h"
#include "nx_azure_iot_hub_client.h"
#include "nx_secure_tls_api.h"
#include "nx_secure_x509.h"
#include "nxd_dhcp_client.h"
#include "nxd_dns.h"
#include "nxd_sntp_client.h"

#include <inttypes.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#ifndef SAMPLE_MANUFACTURER
#error "SAMPLE_MANUFACTURER not defined"
#endif

#ifndef SAMPLE_MODEL
#error "SAMPLE_MODEL not defined"
#endif

/*
 * Note: The PnP model comes from the following sample in the NetX Duo repository:
 * addons/azure_iot/samples/sample_azure_iot_embedded_sdk_pnp.c
 * When we update NetX Duo, we also need to ensure this ID matches the one used in
 * the above file. It refers to models submitted to http://github.com/Azure/iot-plugandplay-models
 */
#define PNP_MODEL "dtmi:com:example:Thermostat;4"

extern VOID sample_connection_monitor(NX_IP *ip_ptr,
                                      NX_AZURE_IOT_HUB_CLIENT *iothub_client_ptr,
                                      UINT connection_status,
                                      UINT (*iothub_init)(NX_AZURE_IOT_HUB_CLIENT *hub_client_ptr));

#define MILLISECONDS_IN_SECOND 1000U

typedef enum app_event_e {
    APP_EVENT_NONE = 0,
    APP_EVENT_SEND_TELEMETRY_PULSE,
    APP_EVENT_SEND_MSG,
    APP_EVENT_STOP_TELEMETRY
} app_event_t;

typedef struct application_msg_s {
    app_event_t event;
    union {
        int32_t return_code;
        const char *text;
    };
} application_msg_t;

// NetX Duo configuration
static void network_stack_setup_thread_entry(void *context);
static bool are_symmetric_key_connection_details_provided(void);
static bool initialize_network_stack(void);
static void ip_stack_configuration_thread_entry(void *context);
static bool configure_ip_address(void);
static bool configure_dns(uint32_t dns_server_address);
static bool sync_sntp_time(void);
static bool sync_sntp_time_internal(uint32_t sntp_server_address);
static UINT fetch_unix_time_callback(ULONG *unix_time);

// NetX Duo Azure IoT Hub Client
static void sample_entry(void);
static void telemetry_thread_entry(void *context);
static bool initialize_root_ca_certificates(void);
static UINT initialize_iothub(NX_AZURE_IOT_HUB_CLIENT *client);
static void on_azure_iot_log(az_log_classification classification, UCHAR *msg, UINT msg_len);
static void on_connection_status_update(NX_AZURE_IOT_HUB_CLIENT *hub_client, UINT status);
static void on_timer_expiry(void *context);
static void on_update_receive_change(NX_AZURE_IOT_ADU_AGENT *adu_agent_ptr,
                                     UINT update_state,
                                     UCHAR *provider,
                                     UINT provider_length,
                                     UCHAR *name,
                                     UINT name_length,
                                     UCHAR *version,
                                     UINT version_length);

static bool enqueue_application_message(const application_msg_t *msg);
static UINT get_connection_status_atomically(void);
static void set_connection_status_atomically(UINT status);

static uint32_t unix_time_base = 0;
static NX_PACKET_POOL packet_pool = {0};
static NX_IP network_ip = {0};
static NX_DNS dns_client = {0};
static NX_DHCP dhcp_client = {0};

static NX_SECURE_X509_CERT root_ca_cert = {0};
static NX_SECURE_X509_CERT root_ca_cert_2 = {0};
static NX_SECURE_X509_CERT root_ca_cert_3 = {0};
static NX_AZURE_IOT nx_azure_iot = {0};
static NX_AZURE_IOT_HUB_CLIENT iothub_client = {0};
static NX_AZURE_IOT_ADU_AGENT iothub_adu_agent = {0};
static UINT connection_status = NX_AZURE_IOT_NOT_INITIALIZED;
static uint8_t nx_azure_iot_tls_metadata_buffer[NX_AZURE_IOT_TLS_METADATA_BUFFER_SIZE] = {0};
static uint32_t nx_azure_iot_thread_stack[8192UL / sizeof(uint32_t)] = {0};

static osMessageQueueId_t msg_queue = NULL;
static osMutexId_t connection_status_mutex = NULL;

int endpoint_init(void)
{
    const osThreadAttr_t thread_attributes = {.priority = osPriorityNormal, .name = "NET_STACK", .stack_size = 4096UL};
    if (osThreadNew(network_stack_setup_thread_entry, NULL, &thread_attributes) == NULL) {
        printf("Failed to create thread\r\n");
        return -1;
    }

    return 0;
}

void mqtt_send_inference_result(const char *message)
{
    if (message == NULL) {
        return;
    }

    size_t msg_length = strlen(message) + 1;

    // The memory is freed when the message is processed by the thread
    char *text = (char *)malloc(msg_length);
    if (text == NULL) {
        return;
    }

    memcpy(text, message, msg_length);

    if (!enqueue_application_message(&(application_msg_t){.event = APP_EVENT_SEND_MSG, .text = text})) {
        free(text);
    }
}

// NetX Duo configuration
static void network_stack_setup_thread_entry(void *context)
{
    (void)context;

    const osThreadAttr_t thread_attributes = {.priority = osPriorityNormal, .name = "IP_STACK", .stack_size = 4096UL};

    bool network_stack_setup_failed = false;

    if (are_symmetric_key_connection_details_provided() == false) {
        printf("Symmetric key connection details have not been provided\n\r");
        network_stack_setup_failed = true;
    }

    if (!network_stack_setup_failed) {
        if (initialize_network_stack() == false) {
            printf("Network stack initialization failed.\n\r");
            network_stack_setup_failed = true;
        }
    }

    if (!network_stack_setup_failed) {
        if (osThreadNew(ip_stack_configuration_thread_entry, NULL, &thread_attributes) == NULL) {
            printf("A new thread to configure the IP stack could not be created\n\r");
            network_stack_setup_failed = true;
        }
    }

    if (network_stack_setup_failed) {
        ml_task_inference_start();
    }

    osThreadExit();
}

static bool are_symmetric_key_connection_details_provided(void)
{
    bool non_default_connection_details =
        !((strcmp(HOST_NAME, DEFAULT_CONNECTION_DETAILS) == 0) || (strcmp(DEVICE_ID, DEFAULT_CONNECTION_DETAILS) == 0)
          || (strcmp(DEVICE_SYMMETRIC_KEY, DEFAULT_CONNECTION_DETAILS) == 0));

    if (non_default_connection_details == false) {
        printf("Please copy the connection string for the device.\r\n");
    }

    return non_default_connection_details;
}

static bool initialize_network_stack(void)
{
    nx_system_initialize();

    static uint32_t packet_pool_buffer[SAMPLE_POOL_SIZE / sizeof(uint32_t)] = {0};
    UINT status = nx_packet_pool_create(
        &packet_pool, "NetX Packet Pool", PACKET_SIZE, (uint8_t *)packet_pool_buffer, sizeof(packet_pool_buffer));

    if (status != NX_SUCCESS) {
        printf("Error (0x%02x): NetX Duo packet pool could not be created.\r\n", status);
        return false;
    }

    static uint32_t network_ip_memory_buffer[2048UL / sizeof(uint32_t)] = {0};

    status = nx_ip_create(&network_ip,
                          "NetX IP Instance",
                          IP_ADDRESS(0, 0, 0, 0),
                          IP_ADDRESS(0, 0, 0, 0),
                          &packet_pool,
                          nx_driver_cmsis_ip_link_fsm,
                          (uint8_t *)network_ip_memory_buffer,
                          sizeof(network_ip_memory_buffer),
                          MAP_THREADX_PRIORITY_TO_CMSIS(osPriorityNormal));
    if (status != NX_SUCCESS) {
        printf("Error (0x%02x): NetX Duo IP instance could not be created.\r\n", status);
        return false;
    }

    status = nx_ip_fragment_enable(&network_ip);
    if (status != NX_SUCCESS) {
        printf("Error (0x%02x): IP fragment processing could not be enabled.\r\n", status);
        return false;
    }

    static uint32_t arp_cache_memory_buffer[512 / sizeof(uint32_t)] = {0};
    status = nx_arp_enable(&network_ip, arp_cache_memory_buffer, sizeof(arp_cache_memory_buffer));
    if (status != NX_SUCCESS) {
        printf("Error (0x%02x): ARP could not be enabled.\r\n", status);
        return false;
    }

    status = nx_tcp_enable(&network_ip);
    if (status != NX_SUCCESS) {
        printf("Error (0x%02x): TCP could not be enabled\r\n", status);
        return false;
    }

    status = nx_udp_enable(&network_ip);
    if (status != NX_SUCCESS) {
        printf("Error (0x%02x): UDP could not be enabled\r\n", status);
        return false;
    }

    nx_secure_tls_initialize();

    return true;
}

static void ip_stack_configuration_thread_entry(void *context)
{
    (void)context;

    if (configure_ip_address() == false) {
        printf("NetX failed to get an IP address!\r\n");
        osThreadExit();
        return;
    }

    uint32_t dns_server_address[3] = {0};
    size_t dns_server_address_size = sizeof(dns_server_address);
    nx_dhcp_interface_user_option_retrieve(
        &dhcp_client, 0, NX_DHCP_OPTION_DNS_SVR, (uint8_t *)(dns_server_address), &dns_server_address_size);

    if (configure_dns(dns_server_address[0]) == false) {
        printf("Failed to create DNS.\r\n");
        osThreadExit();
        return;
    }

    if (sync_sntp_time() == false) {
        printf("Failed to sync SNTP time.\r\n");
        printf("Set Time to default value: 0x%02x.", SAMPLE_SYSTEM_TIME);
        unix_time_base = SAMPLE_SYSTEM_TIME;
    } else {
        printf("SNTP Time successfully synchronized.\r\n");
    }

    ULONG unix_time = SAMPLE_SYSTEM_TIME;
    fetch_unix_time_callback(&unix_time);
    srand(unix_time);

    sample_entry();
}

static bool configure_ip_address(void)
{
    printf("DHCP In Progress...\r\n");

    UINT status = nx_dhcp_create(&dhcp_client, &network_ip, "DHCP Client");
    if (status != NX_SUCCESS) {
        printf("Error (0x%02x): DHCP client could not be created\r\n", status);
        return false;
    }

    status = nx_dhcp_user_option_request(&dhcp_client, NX_DHCP_OPTION_NTP_SVR);
    if (status != NX_SUCCESS) {
        printf("Error (0x%02x): DHCP user option could not be obtained\r\n", status);
        nx_dhcp_delete(&dhcp_client);
        return false;
    }

    status = nx_dhcp_start(&dhcp_client);
    if (status != NX_SUCCESS) {
        printf("Error (0x%02x): DHCP client could not be started\r\n", status);
        nx_dhcp_delete(&dhcp_client);
        return false;
    }

    ULONG actual_status = UINT32_MAX;
    const ULONG needed_status = NX_IP_ADDRESS_RESOLVED;
    status = nx_ip_status_check(&network_ip, needed_status, &actual_status, DHCP_WAIT_TIME);
    if ((status != NX_SUCCESS) && (actual_status != needed_status)) {
        nx_dhcp_delete(&dhcp_client);
        printf("Error (0x%02x): Could not resolve DHCP. Expected `0x%02x` but got `0x%02x`.\r\n",
               status,
               (UINT)needed_status,
               (UINT)actual_status);
        return false;
    }

    return true;
}

static bool configure_dns(uint32_t dns_server_address)
{
    // Create a DNS instance for the Client.  Note this function will create
    // the DNS Client packet pool for creating DNS message packets intended
    // for querying its DNS server.
    UINT status = nx_dns_create(&dns_client, &network_ip, (UCHAR *)"DNS Client");
    if (status != NX_SUCCESS) {
        printf("Error (0x%02x): DNS client could not be created\r\n", status);
        return false;
    }

    // Add an IPv4 server address to the Client list.
    status = nx_dns_server_add(&dns_client, dns_server_address);
    if (status != NX_SUCCESS) {
        printf("Error (0x%02x): DNS client could not add a server\r\n", status);
        nx_dns_delete(&dns_client);
        return false;
    }

    return true;
}

static bool sync_sntp_time(void)
{
    static const char *sntp_servers[] = {
        "0.pool.ntp.org",
        "1.pool.ntp.org",
        "2.pool.ntp.org",
        "3.pool.ntp.org",
    };
    ULONG sntp_server_address[3] = {0};
    size_t sntp_server_address_size = sizeof(sntp_server_address);

    UINT user_option_retrieved = nx_dhcp_interface_user_option_retrieve(
        &dhcp_client, 0, NX_DHCP_OPTION_NTP_SVR, (UCHAR *)(sntp_server_address), &sntp_server_address_size);

    if (user_option_retrieved == NX_SUCCESS) {
        for (size_t i = 0; (i * sizeof(uint32_t)) < sntp_server_address_size; i++) {
            // Start SNTP to sync the local time.
            if (sync_sntp_time_internal(sntp_server_address[i]) == NX_SUCCESS) {
                return true;
            }
        }
    }

    static uint32_t sntp_server_index = 0;
    // Sync time by NTP server array.
    for (uint32_t i = 0; i < SNTP_MAX_SYNC_CHECK; i++) {
        // Make sure the network is still valid by obtaining the gateway address
        ULONG gateway_address = 0;
        while (nx_ip_gateway_address_get(&network_ip, &gateway_address)) {
            osDelay(NX_IP_PERIODIC_RATE);
        }

        UINT status = nx_dns_host_by_name_get(
            &dns_client, (uint8_t *)sntp_servers[sntp_server_index], &sntp_server_address[0], 5 * NX_IP_PERIODIC_RATE);

        if (status == NX_SUCCESS) {
            if (sync_sntp_time_internal(sntp_server_address[0])) {
                return true;
            }
        }

        // Switch SNTP server every time.
        sntp_server_index = (sntp_server_index + 1) % (sizeof(sntp_servers) / sizeof(sntp_servers[0]));
    }

    return false;
}

static bool sync_sntp_time_internal(uint32_t sntp_server_address)
{
    static NX_SNTP_CLIENT sntp_client = {0};

    UINT status = nx_sntp_client_create(&sntp_client, &network_ip, 0, &packet_pool, NX_NULL, NX_NULL, NX_NULL);
    if (status != NX_SUCCESS) {
        printf("Error (0x%02x): SNTP client could not be created\r\n", status);
        return false;
    }

    status = nx_sntp_client_initialize_unicast(&sntp_client, sntp_server_address);
    if (status != NX_SUCCESS) {
        printf("Error (0x%02x): SNTP client could not initialize unicast\r\n", status);
        nx_sntp_client_delete(&sntp_client);
        return false;
    }

    status = nx_sntp_client_set_local_time(&sntp_client, 0, 0);
    if (status != NX_SUCCESS) {
        printf("Error (0x%02x): SNTP client could not set local time\r\n", status);
        nx_sntp_client_delete(&sntp_client);
        return false;
    }

    status = nx_sntp_client_run_unicast(&sntp_client);
    if (status != NX_SUCCESS) {
        printf("Error (0x%02x): SNTP client could not run unicast\r\n", status);
        nx_sntp_client_stop(&sntp_client);
        nx_sntp_client_delete(&sntp_client);
        return false;
    }

    // Wait until updates are received or timeout
    for (uint32_t i = 0; i < SNTP_MAX_UPDATE_CHECK; i++) {
        // Verify we have a valid SNTP service running.
        UINT server_status = NX_FALSE;
        status = nx_sntp_client_receiving_updates(&sntp_client, &server_status);
        if ((status == NX_SUCCESS) && (server_status == NX_TRUE)) {
            ULONG sntp_seconds = 0;
            ULONG sntp_fraction = 0;
            status = nx_sntp_client_get_local_time(&sntp_client, &sntp_seconds, &sntp_fraction, NX_NULL);
            if (status != NX_SUCCESS) {
                printf("Error (0x%02x): SNTP client could not get local time\r\n", status);
                continue;
            }

            uint32_t system_time_in_second = osKernelGetTickCount() / TX_TIMER_TICKS_PER_SECOND;

            // Convert to Unix epoch and minus the current system time.
            unix_time_base = (sntp_seconds - (system_time_in_second + SAMPLE_UNIX_TO_NTP_EPOCH_SECOND));

            // Time sync successfully.
            nx_sntp_client_stop(&sntp_client);
            nx_sntp_client_delete(&sntp_client);

            return true;
        }

        osDelay(SNTP_UPDATE_SLEEP_INTERVAL);
    }

    // Time sync failed.

    nx_sntp_client_stop(&sntp_client);
    nx_sntp_client_delete(&sntp_client);

    return false;
}

static UINT fetch_unix_time_callback(ULONG *unix_time)
{
    *unix_time = unix_time_base + (osKernelGetTickCount() / TX_TIMER_TICKS_PER_SECOND);
    return NX_SUCCESS;
}

// NetX Duo Azure IoT Hub Client
static void sample_entry(void)
{
    nx_azure_iot_log_init(on_azure_iot_log);

    UINT status = nx_azure_iot_create(&nx_azure_iot,
                                      (uint8_t *)"Azure IoT",
                                      &network_ip,
                                      &packet_pool,
                                      &dns_client,
                                      nx_azure_iot_thread_stack,
                                      sizeof(nx_azure_iot_thread_stack),
                                      MAP_THREADX_PRIORITY_TO_CMSIS(osPriorityNormal),
                                      fetch_unix_time_callback);
    if (status != NX_AZURE_IOT_SUCCESS) {
        printf("Error (0x%02x): Failed to create Azure IoT handler!\r\n", status);
        return;
    }

    if (initialize_root_ca_certificates() == false) {
        printf("Failed to initialize ROOT CA certificates!\r\n");
        nx_azure_iot_delete(&nx_azure_iot);
        return;
    }

    status = initialize_iothub(&iothub_client);
    if (status != NX_AZURE_IOT_SUCCESS) {
        printf("Error (0x%02x): Failed to initialize iothub client\r\n", status);
        connection_status = NX_AZURE_IOT_NOT_INITIALIZED;
    }

    connection_status_mutex = osMutexNew(NULL);

    if (connection_status_mutex == NULL) {
        printf("Error: Failure to create a mutex for `connection_status_mutex`!\r\n");
        return;
    }

    nx_azure_iot_hub_client_connect(&iothub_client, NX_TRUE, NX_WAIT_FOREVER);

    UINT read_connection_status = get_connection_status_atomically();
    if (read_connection_status != NX_AZURE_IOT_SUCCESS) {
        printf("Error (0x%02x): Failed on nx_azure_iot_hub_client_connect!\r\n", read_connection_status);
    }

    psa_fwu_component_info_t info_ns;
    psa_status_t psa_status = psa_fwu_query(FWU_COMPONENT_ID_NONSECURE, &info_ns);
    if (psa_status != PSA_SUCCESS) {
        printf("Failed to query non-secure firmware information. Error %" PRId32 "\r\n", psa_status);
        return;
    }
    char version[16];
    int length = snprintf(version,
                          sizeof(version),
                          "%" PRIu16 ".%" PRIu16 ".%" PRIu16,
                          (uint16_t)info_ns.version.major,
                          (uint16_t)info_ns.version.minor,
                          info_ns.version.patch);
    printf("Firmware version: %s\r\n", version);

    status = nx_azure_iot_adu_agent_start(&iothub_adu_agent,
                                          &iothub_client,
                                          (UCHAR *)SAMPLE_MANUFACTURER,
                                          sizeof(SAMPLE_MANUFACTURER) - 1,
                                          (UCHAR *)SAMPLE_MODEL,
                                          sizeof(SAMPLE_MODEL) - 1,
                                          (UCHAR *)version,
                                          length,
                                          on_update_receive_change,
                                          nx_azure_iot_adu_agent_driver_ns);

    if (status == NX_AZURE_IOT_SUCCESS) {
        printf("Azure Device Update agent started\r\n");
    } else {
        printf("Failed to start Azure Device Update agent! error code = 0x%08x\r\n", status);
    }

    const osThreadAttr_t thread_attributes = {
        .priority = osPriorityNormal1, .name = "TLMY_STACK", .stack_size = 4096UL};

    if (osThreadNew(telemetry_thread_entry, NULL, &thread_attributes) == NULL) {
        printf("Failed to create telemetry sample thread!\r\n");
    }

    while (true) {
        read_connection_status = get_connection_status_atomically();
        sample_connection_monitor(&network_ip, &iothub_client, read_connection_status, initialize_iothub);

        osDelay(NX_IP_PERIODIC_RATE);
    }

    nx_azure_iot_hub_client_deinitialize(&iothub_client);
    nx_azure_iot_delete(&nx_azure_iot);
}

static void telemetry_thread_entry(void *context)
{
    (void)context;

    msg_queue = osMessageQueueNew(5, sizeof(application_msg_t), NULL);
    if (msg_queue == NULL) {
        printf("Failed to create an application message queue!");
        goto exit;
    }

    static osTimerId_t pulse_timer = NULL;
    // We avoid periodic timer as depending on the load, it might saturate the event queue.
    // the timer is restarted manually once the telemetry pulse has been sent.
    pulse_timer = osTimerNew(on_timer_expiry, osTimerOnce, NULL, NULL);
    if (pulse_timer == NULL) {
        printf("Creating telemetry timer failed!");
        goto exit;
    }

    ml_task_inference_start();

    osStatus_t os_status = osTimerStart(pulse_timer, osKernelGetTickFreq());
    if (os_status != osOK) {
        printf("Failed to start OS timer failed with code: %d", os_status);
        goto exit;
    }

    const uint32_t kernel_tick_freq_hz = osKernelGetTickFreq();

    uint32_t timeout = (20 * kernel_tick_freq_hz) / MILLISECONDS_IN_SECOND;

    if (kernel_tick_freq_hz < 50) {
        printf("Kernel tick length is longer than 20ms, this might cause issues with thread dispatching\r\n");
        timeout += 1; // wait at least one tick
    }

    application_msg_t message = {0};
    while (true) {
        UINT read_connection_status = get_connection_status_atomically();
        if (read_connection_status != NX_AZURE_IOT_SUCCESS) {
            osDelay(NX_IP_PERIODIC_RATE);
            continue;
        }

        if (osMessageQueueGet(msg_queue, &message, NULL, timeout) != osOK) {
            message.event = APP_EVENT_NONE;
        }

        switch (message.event) {
            case APP_EVENT_SEND_MSG: {
                printf("Sending message %s\r\n", message.text);

                NX_PACKET *packet = NULL;
                UINT status =
                    nx_azure_iot_hub_client_telemetry_message_create(&iothub_client, &packet, NX_WAIT_FOREVER);
                if (status != NX_AZURE_IOT_SUCCESS) {
                    printf("Failed to create telemetry message to send ML inference! Error code = 0x%08x\r\n", status);
                    free((void *)message.text);
                    goto exit;
                }

                // On successful return of `nx_azure_iot_hub_client_telemetry_send()`,
                // memory of `NX_PACKET` is released
                status = nx_azure_iot_hub_client_telemetry_send(
                    &iothub_client, packet, (UCHAR *)message.text, strlen(message.text) + 1, NX_WAIT_FOREVER);

                if (status == NX_AZURE_IOT_SUCCESS) {
                    printf("Message sent\r\n");
                } else {
                    printf("Failed to send telemetry message containing ML inference! Error code = 0x%08x\r\n", status);
                    nx_azure_iot_hub_client_telemetry_message_delete(packet);
                }

                // Free memory allocated when when the message was generated
                free((void *)message.text);

                break;
            }

            case APP_EVENT_SEND_TELEMETRY_PULSE: {
                printf("Sending telemetry pulse\r\n");

                NX_PACKET *packet = NULL;
                UINT status =
                    nx_azure_iot_hub_client_telemetry_message_create(&iothub_client, &packet, NX_WAIT_FOREVER);
                if (status != NX_AZURE_IOT_SUCCESS) {
                    printf("Failed to create telemetry message to send pulse! Error code = 0x%08x\r\n", status);
                    goto exit;
                }

                static const CHAR *s_property[] = {"propertyA", "valueA"};
                status = nx_azure_iot_hub_client_telemetry_property_add(packet,
                                                                        (UCHAR *)s_property[0],
                                                                        (USHORT)strlen(s_property[0]),
                                                                        (UCHAR *)s_property[1],
                                                                        (USHORT)strlen(s_property[1]),
                                                                        NX_WAIT_FOREVER);
                if (status != NX_AZURE_IOT_SUCCESS) {
                    printf("Failed to add property to telemetry message to send pulse! Error code = 0x%08x\r\n",
                           status);
                    goto exit;
                }

                const char *text = "pulse";
                // On successful return of `nx_azure_iot_hub_client_telemetry_send()`,
                // memory of `NX_PACKET` is released
                status = nx_azure_iot_hub_client_telemetry_send(
                    &iothub_client, packet, (UCHAR *)text, strlen(text) + 1, NX_WAIT_FOREVER);

                if (status != NX_AZURE_IOT_SUCCESS) {
                    printf("Failed to send telemetry message containing ML inference! Error code = 0x%08x\r\n", status);
                    nx_azure_iot_hub_client_telemetry_message_delete(packet);
                }

                // restart timer for next telemetry pulse
                os_status = osTimerStart(pulse_timer, osKernelGetTickFreq());
                if (os_status != osOK) {
                    printf("Failed to restart OS timer failed with code: %d", os_status);
                    goto exit;
                }

                break;
            }

            case APP_EVENT_STOP_TELEMETRY:
                goto exit;

            default:
                break;
        }
    }

exit:
    osTimerDelete(pulse_timer);
    osThreadExit();
}

static bool initialize_root_ca_certificates(void)
{
    UINT status = nx_secure_x509_certificate_initialize(&root_ca_cert,
                                                        (UCHAR *)_nx_azure_iot_root_cert,
                                                        (USHORT)_nx_azure_iot_root_cert_size,
                                                        NX_NULL,
                                                        0,
                                                        NULL,
                                                        0,
                                                        NX_SECURE_X509_KEY_TYPE_NONE);
    if (status != NX_SECURE_X509_SUCCESS) {
        printf("Error (0x%02x): Failed to initialize Root Cert 1\r\n", status);
        return false;
    }

    status = nx_secure_x509_certificate_initialize(&root_ca_cert_2,
                                                   (UCHAR *)_nx_azure_iot_root_cert_2,
                                                   (USHORT)_nx_azure_iot_root_cert_size_2,
                                                   NX_NULL,
                                                   0,
                                                   NULL,
                                                   0,
                                                   NX_SECURE_X509_KEY_TYPE_NONE);
    if (status != NX_SECURE_X509_SUCCESS) {
        printf("Error (0x%02x): Failed to initialize Root Cert 2\r\n", status);
        return false;
    }

    status = nx_secure_x509_certificate_initialize(&root_ca_cert_3,
                                                   (UCHAR *)_nx_azure_iot_root_cert_3,
                                                   (USHORT)_nx_azure_iot_root_cert_size_3,
                                                   NX_NULL,
                                                   0,
                                                   NULL,
                                                   0,
                                                   NX_SECURE_X509_KEY_TYPE_NONE);
    if (status != NX_SECURE_X509_SUCCESS) {
        printf("Error (0x%02x): Failed to initialize Root Cert 3\r\n", status);
        return false;
    }

    return true;
}

static UINT initialize_iothub(NX_AZURE_IOT_HUB_CLIENT *client)
{
    UCHAR *iothub_hostname = (UCHAR *)HOST_NAME;
    UCHAR *iothub_device_id = (UCHAR *)DEVICE_ID;
    UINT iothub_hostname_length = sizeof(HOST_NAME) - 1;
    UINT iothub_device_id_length = sizeof(DEVICE_ID) - 1;

    printf("IoTHub Host Name: %.*s; Device ID: %.*s.\r\n",
           iothub_hostname_length,
           iothub_hostname,
           iothub_device_id_length,
           iothub_device_id);

    UINT status = nx_azure_iot_hub_client_initialize(client,
                                                     &nx_azure_iot,
                                                     iothub_hostname,
                                                     iothub_hostname_length,
                                                     iothub_device_id,
                                                     iothub_device_id_length,
                                                     (UCHAR *)MODULE_ID,
                                                     sizeof(MODULE_ID) - 1,
                                                     _nx_azure_iot_tls_supported_crypto,
                                                     _nx_azure_iot_tls_supported_crypto_size,
                                                     _nx_azure_iot_tls_ciphersuite_map,
                                                     _nx_azure_iot_tls_ciphersuite_map_size,
                                                     nx_azure_iot_tls_metadata_buffer,
                                                     sizeof(nx_azure_iot_tls_metadata_buffer),
                                                     &root_ca_cert);
    if (status != NX_AZURE_IOT_SUCCESS) {
        printf("Failed on nx_azure_iot_hub_client_initialize!: error code = 0x%08x\r\n", status);
        return (status);
    }

    status = nx_azure_iot_hub_client_model_id_set(client, (const UCHAR *)PNP_MODEL, sizeof(PNP_MODEL) - 1);
    if (status != NX_AZURE_IOT_SUCCESS) {
        printf("Failed on nx_azure_iot_hub_client_model_id_set!: error code = 0x%08x\r\n", status);
        goto end;
    }

    status = nx_azure_iot_hub_client_properties_enable(client);
    if (status != NX_AZURE_IOT_SUCCESS) {
        printf("Properties enable failed!: error code = 0x%08x\r\n", status);
        goto end;
    }

    status = nx_azure_iot_hub_client_trusted_cert_add(client, &root_ca_cert_2);
    if (status != NX_AZURE_IOT_SUCCESS) {
        printf("Failed on nx_azure_iot_hub_client_trusted_cert_add!: error code = 0x%08x\r\n", status);
        goto end;
    }

    status = nx_azure_iot_hub_client_trusted_cert_add(client, &root_ca_cert_3);
    if (status != NX_AZURE_IOT_SUCCESS) {
        printf("Failed on nx_azure_iot_hub_client_trusted_cert_add!: error code = 0x%08x\r\n", status);
        goto end;
    }

    status = nx_azure_iot_hub_client_symmetric_key_set(
        client, (UCHAR *)DEVICE_SYMMETRIC_KEY, sizeof(DEVICE_SYMMETRIC_KEY) - 1);
    if (status != NX_AZURE_IOT_SUCCESS) {
        printf("Failed on nx_azure_iot_hub_client_symmetric_key_set!\r\n");
        goto end;
    }

    status = nx_azure_iot_hub_client_connection_status_callback_set(client, on_connection_status_update);
    if (status != NX_AZURE_IOT_SUCCESS) {
        printf("Failed to set connection status callback!\r\n");
        goto end;
    }

end:
    if (status == NX_AZURE_IOT_SUCCESS) {
        printf("Waiting to connect to IoTHub...\r\n");
    } else {
        nx_azure_iot_hub_client_deinitialize(client);
    }

    return status;
}

static void on_azure_iot_log(az_log_classification classification, UCHAR *msg, UINT msg_len)
{
    if (classification == AZ_LOG_IOT_AZURERTOS) {
        printf("%.*s", msg_len, (char *)msg);
    }
}

static void on_connection_status_update(NX_AZURE_IOT_HUB_CLIENT *hub_client, UINT status)
{
    (void)hub_client;

    if (status) {
        printf("Disconnected from IoTHub!: error code = 0x%08x\r\n", status);
    } else {
        printf("Connected to IoTHub.\r\n");
    }

    set_connection_status_atomically(status);
}

static void on_timer_expiry(void *context)
{
    (void)context;

    enqueue_application_message(&(application_msg_t){.event = APP_EVENT_SEND_TELEMETRY_PULSE, .return_code = 0});
}

static void on_update_receive_change(NX_AZURE_IOT_ADU_AGENT *adu_agent_ptr,
                                     UINT update_state,
                                     UCHAR *provider,
                                     UINT provider_length,
                                     UCHAR *name,
                                     UINT name_length,
                                     UCHAR *version,
                                     UINT version_length)
{
    if (update_state == NX_AZURE_IOT_ADU_AGENT_UPDATE_RECEIVED) {
        ml_task_inference_stop();
        enqueue_application_message(&(application_msg_t){.event = APP_EVENT_STOP_TELEMETRY, .return_code = 0});
        printf("Received new update: Provider: %.*s; Name: %.*s, Version: %.*s\r\n",
               provider_length,
               provider,
               name_length,
               name,
               version_length,
               version);
        /* Start to download and install update immediately for testing.  */
        nx_azure_iot_adu_agent_update_download_and_install(adu_agent_ptr);
    } else if (update_state == NX_AZURE_IOT_ADU_AGENT_UPDATE_INSTALLED) {
        /* Start to apply update immediately for testing.  */
        nx_azure_iot_adu_agent_update_apply(adu_agent_ptr);
    }
}

static bool enqueue_application_message(const application_msg_t *msg)
{
    if (msg_queue == NULL) {
        return false;
    }

    if (osMessageQueuePut(msg_queue, msg, 0, 0) != osOK) {
        printf("Failed to send message to app_msg_queue\r\n");
        return false;
    }

    return true;
}

static UINT get_connection_status_atomically(void)
{
    osMutexAcquire(connection_status_mutex, osWaitForever);
    UINT read_status = connection_status;
    osMutexRelease(connection_status_mutex);

    return read_status;
}

static void set_connection_status_atomically(UINT status)
{
    osMutexAcquire(connection_status_mutex, osWaitForever);
    connection_status = status;
    osMutexRelease(connection_status_mutex);
}
