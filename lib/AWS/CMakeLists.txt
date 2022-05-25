# Copyright (c) 2021 ARM Limited. All rights reserved.
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.18)

FetchContent_GetProperties(aws-iot-device-sdk)
FetchContent_GetProperties(tf-m)

add_subdirectory(coreMQTT-Agent)
set(coremqtt-agent_SOURCE_DIR ${coremqtt-agent_SOURCE_DIR} PARENT_SCOPE)

add_subdirectory(tinycbor)
add_subdirectory(ota_for_aws)
add_subdirectory(PKCS11)

add_library(AWS-extra STATIC EXCLUDE_FROM_ALL)

target_include_directories(AWS-extra
    PUBLIC
        "${PRJ_DIR}/bsp/aws_libraries/3rdparty/jsmn"
        "${PRJ_DIR}/bsp/aws_libraries/3rdparty/mbedtls_config"
        "${PRJ_DIR}/bsp/aws_libraries/3rdparty/mbedtls_utils"
        "${PRJ_DIR}/bsp/aws_libraries/3rdparty/lwip_osal/include"
        "${PRJ_DIR}/bsp/aws_libraries/abstractions/mqtt_agent/include"
        "${PRJ_DIR}/bsp/aws_libraries/abstractions/platform/include"
        "${PRJ_DIR}/bsp/aws_libraries/abstractions/platform/include/platform"
        "${PRJ_DIR}/bsp/aws_libraries/abstractions/platform/include/types"
        "${PRJ_DIR}/bsp/aws_libraries/abstractions/secure_sockets/include"
        "${PRJ_DIR}/bsp/aws_libraries/abstractions/transport/secure_sockets"
        "${PRJ_DIR}/bsp/aws_libraries/c_sdk/standard/common/include"
        "${PRJ_DIR}/bsp/aws_libraries/c_sdk/standard/common/include/types"
        "${PRJ_DIR}/bsp/aws_libraries/c_sdk/standard/mqtt/include"
        "${PRJ_DIR}/bsp/aws_libraries/c_sdk/standard/mqtt/include/types"
        "${PRJ_DIR}/bsp/aws_libraries/c_sdk/standard/mqtt/src/private"
        "${PRJ_DIR}/bsp/aws_libraries/demos/common/mqtt_demo_helpers"
        "${PRJ_DIR}/bsp/aws_libraries/demos/common/mqtt_subscription_manager"
        "${PRJ_DIR}/bsp/aws_libraries/demos/coreMQTT_Agent"
        "${PRJ_DIR}/bsp/aws_libraries/demos/dev_mode_key_provisioning/include"
        "${PRJ_DIR}/bsp/aws_libraries/demos/network_manager"
        "${PRJ_DIR}/bsp/aws_libraries/freertos_kernel/include"
        "${PRJ_DIR}/bsp/aws_libraries/freertos_plus/standard/tls/include"
        "${PRJ_DIR}/kws"
        "${PRJ_DIR}/kws/include"
        "${TFM_PLATFORM_TARGET_DIR}/arm/mps3/an547"
        "${TFM_PLATFORM_TARGET_DIR}/arm/mps3/an547/cmsis_drivers"
        "${TFM_PLATFORM_TARGET_DIR}/arm/mps3/an547/cmsis_drivers/config"
        "${TFM_PLATFORM_TARGET_DIR}/arm/mps3/an547/device"
        "${TFM_PLATFORM_TARGET_DIR}/arm/mps3/an547/device/config"
        "${TFM_PLATFORM_TARGET_DIR}/arm/mps3/an547/device/source/armclang"
        "${TFM_PLATFORM_TARGET_DIR}/arm/mps3/an547/native_drivers"
        "${TFM_PLATFORM_TARGET_DIR}/arm/mps3/an547/partition"
        "${TFM_PLATFORM_TARGET_DIR}/arm/mps3/an547/services/src"
        "${TFM_PLATFORM_TARGET_DIR}/arm/mps3/common"
        "${tf-m_SOURCE_DIR}/interface/include"
        "${PRJ_DIR}/lib/AWS/ota_for_aws"
)

target_sources(AWS-extra
    PRIVATE
        # OTA
        "${PRJ_DIR}/kws/ota/ota_demo_core_mqtt.c"
        "${PRJ_DIR}/kws/ota/ota_pal_psa/version/application_version.c"
        "${PRJ_DIR}/kws/ota/ota_pal_psa/ota_pal.c"
        "${PRJ_DIR}/kws/ota/provision/ota_provision.c"

        # MQTT agent
        "${PRJ_DIR}/bsp/aws_libraries/abstractions/mqtt_agent/freertos_agent_message.c"
        "${PRJ_DIR}/bsp/aws_libraries/abstractions/mqtt_agent/freertos_command_pool.c"

        # common
        "${PRJ_DIR}/bsp/aws_libraries/c_sdk/standard/common/iot_device_metrics.c"
        "${PRJ_DIR}/bsp/aws_libraries/c_sdk/standard/common/iot_init.c"
        "${PRJ_DIR}/bsp/aws_libraries/c_sdk/standard/common/iot_static_memory_common.c"
        "${PRJ_DIR}/bsp/aws_libraries/c_sdk/standard/common/taskpool/iot_taskpool.c"
        "${PRJ_DIR}/bsp/aws_libraries/c_sdk/standard/common/taskpool/iot_taskpool_static_memory.c"

        # Logging
        "${PRJ_DIR}/bsp/aws_libraries/logging/iot_logging.c"
        "${PRJ_DIR}/bsp/aws_libraries/logging/iot_logging_task_dynamic_buffers.c"

        #FreeRTOS abstraction
        "${PRJ_DIR}/bsp/aws_libraries/abstractions/platform/freertos/iot_metrics.c"
        "${PRJ_DIR}/bsp/aws_libraries/abstractions/platform/freertos/iot_network_freertos.c"
        "${PRJ_DIR}/bsp/aws_libraries/abstractions/platform/freertos/iot_threads_freertos.c"

        # Clock abstraction
        "${PRJ_DIR}/bsp/aws_libraries/abstractions/platform/freertos/iot_clock_freertos.c"

        # Mbedtls utils and error
        "${PRJ_DIR}/bsp/aws_libraries/3rdparty/mbedtls_utils/mbedtls_error.c"
        "${PRJ_DIR}/bsp/aws_libraries/3rdparty/mbedtls_utils/mbedtls_utils.c"

        # Secure socket
        "${PRJ_DIR}/bsp/aws_libraries/abstractions/transport/secure_sockets/transport_secure_sockets.c"
        "${PRJ_DIR}/bsp/aws_libraries/abstractions/secure_sockets/lwip/iot_secure_sockets.c"

        # Demo code
        "${PRJ_DIR}/bsp/aws_libraries/demos/demo_runner/iot_demo_runner.c"
        "${PRJ_DIR}/bsp/aws_libraries/demos/demo_runner/iot_demo_freertos.c"
        "${PRJ_DIR}/bsp/aws_libraries/demos/common/mqtt_subscription_manager/mqtt_subscription_manager.c"
        "${PRJ_DIR}/bsp/aws_libraries/demos/common/ota_demo_helpers/ota_application_version.c"
        "${PRJ_DIR}/bsp/aws_libraries/demos/dev_mode_key_provisioning/src/aws_dev_mode_key_provisioning.c"
        "${PRJ_DIR}/bsp/aws_libraries/demos/network_manager/aws_iot_network_manager.c"
)

target_link_libraries(AWS-extra
    coreMQTT-Agent
    coreMQTT
    coreHTTP
    PKCS11
    ota_for_aws
    backoffAlgorithm
    tfm-ns-interface
    lwip-sockets
    lwipcore
    lwipopts
    iotsdk-ip-network-api
    cmsis-sockets-api
    mcu-driver-bootstrap
    mcu-driver-hal
    cmsis-rtos-api
)
