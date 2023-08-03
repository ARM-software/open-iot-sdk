# Copyright (c) 2021-2023 ARM Limited. All rights reserved.
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.18)

FetchContent_GetProperties(aws-iot-device-sdk-embedded-c)
FetchContent_GetProperties(trusted-firmware-m)

target_compile_definitions(coremqtt-agent-config
    INTERFACE
        MQTT_AGENT_DO_NOT_USE_CUSTOM_CONFIG
)

add_subdirectory(tinycbor)
add_subdirectory(ota_for_aws)
add_subdirectory(PKCS11)

add_library(AWS-extra STATIC EXCLUDE_FROM_ALL)
set_property(TARGET AWS-extra PROPERTY C_EXTENSIONS ON)

target_compile_definitions(AWS-extra
    PRIVATE
        # FreeRTOS OTA PSA PAL configuration
        OTA_PAL_CODE_SIGNING_ALGO=OTA_PAL_CODE_SIGNING_RSA
        OTA_PAL_SIGNATURE_FORMAT=OTA_PAL_SIGNATURE_RAW
)

target_include_directories(AWS-extra
    PUBLIC
        "aws_libraries/abstractions/platform/include"
        "aws_libraries/abstractions/platform/include/platform"
        "aws_libraries/abstractions/platform/include/types"
        "aws_libraries/abstractions/secure_sockets/include"
        "aws_libraries/abstractions/transport/secure_sockets"
        "aws_libraries/c_sdk/standard/common/include"
        "aws_libraries/c_sdk/standard/common/include/types"
        "aws_libraries/c_sdk/standard/mqtt/include"
        "aws_libraries/c_sdk/standard/mqtt/include/types"
        "aws_libraries/c_sdk/standard/mqtt/src/private"
        "aws_libraries/demos/common/mqtt_demo_helpers"
        "aws_libraries/demos/dev_mode_key_provisioning/include"
        "aws_libraries/demos/network_manager"
        "aws_libraries/freertos_kernel/include"
        "aws_libraries/freertos_plus/standard/tls/include"
        "aws_libraries/freertos_plus/standard/crypto/include"
        "${TFM_PLATFORM_TARGET_DIR}/arm/mps3/an552"
        "${TFM_PLATFORM_TARGET_DIR}/arm/mps3/an552/cmsis_drivers"
        "${TFM_PLATFORM_TARGET_DIR}/arm/mps3/an552/cmsis_drivers/config"
        "${TFM_PLATFORM_TARGET_DIR}/arm/mps3/an552/device"
        "${TFM_PLATFORM_TARGET_DIR}/arm/mps3/an552/device/config"
        "${TFM_PLATFORM_TARGET_DIR}/arm/mps3/an552/device/source/armclang"
        "${TFM_PLATFORM_TARGET_DIR}/arm/mps3/an552/native_drivers"
        "${TFM_PLATFORM_TARGET_DIR}/arm/mps3/an552/partition"
        "${TFM_PLATFORM_TARGET_DIR}/arm/mps3/an552/services/src"
        "${TFM_PLATFORM_TARGET_DIR}/arm/mps3/common"
        "${trusted-firmware-m_SOURCE_DIR}/interface/include"
        "${CMAKE_SOURCE_DIR}/lib/AWS/ota_for_aws"
)

target_sources(AWS-extra
    PRIVATE
        # OTA
        "ota/ota_demo_core_mqtt.c"
        "ota/ota_pal_psa/version/application_version.c"
        "ota/ota_pal_psa/ota_pal.c"
        "ota/provision/ota_provision.c"

        # common
        "aws_libraries/c_sdk/standard/common/iot_device_metrics.c"
        "aws_libraries/c_sdk/standard/common/iot_init.c"
        "aws_libraries/c_sdk/standard/common/iot_static_memory_common.c"
        "aws_libraries/c_sdk/standard/common/taskpool/iot_taskpool.c"
        "aws_libraries/c_sdk/standard/common/taskpool/iot_taskpool_static_memory.c"

        # Logging
        "aws_libraries/logging/iot_logging.c"
        "aws_libraries/logging/iot_logging_task_dynamic_buffers.c"

        #FreeRTOS abstraction
        "aws_libraries/abstractions/platform/freertos/iot_metrics.c"
        "aws_libraries/abstractions/platform/freertos/iot_network_freertos.c"
        "aws_libraries/abstractions/platform/freertos/iot_threads_freertos.c"

        # Clock abstraction
        "aws_libraries/abstractions/platform/freertos/iot_clock_freertos.c"

        # Mbedtls utils and error
        "aws_libraries/3rdparty/mbedtls_utils/mbedtls_error.c"
        "aws_libraries/3rdparty/mbedtls_utils/mbedtls_utils.c"

        # Additional crypto
        "${CMAKE_SOURCE_DIR}/lib/AWS/aws_libraries/freertos_plus/standard/crypto/src/iot_crypto.c"
        "${CMAKE_SOURCE_DIR}/lib/AWS/aws_libraries/freertos_plus/standard/tls/src/iot_tls.c"

        # Secure socket
        "aws_libraries/abstractions/transport/secure_sockets/transport_secure_sockets.c"
        "aws_libraries/abstractions/secure_sockets/lwip/iot_secure_sockets.c"

        # Demo code
        "aws_libraries/demos/demo_runner/iot_demo_runner.c"
        "aws_libraries/demos/demo_runner/iot_demo_freertos.c"
        "aws_libraries/demos/common/ota_demo_helpers/ota_application_version.c"
        "aws_libraries/demos/dev_mode_key_provisioning/src/aws_dev_mode_key_provisioning.c"
        "aws_libraries/demos/network_manager/aws_iot_network_manager.c"
)

target_link_libraries(AWS-extra
    coremqtt-agent
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
    mbed-atomic
    cmsis-rtos-api
    mbedtls-config
)
