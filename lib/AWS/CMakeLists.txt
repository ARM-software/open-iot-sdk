# Copyright (c) 2021 ARM Limited. All rights reserved.
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.18)

set(LIB_NAME lib-AWS-client)

include("${CMAKE_CURRENT_SOURCE_DIR}/aws-iot-device-sdk-fetch.cmake")

add_subdirectory(FreeRTOS-Plus-TCP)
set(freertos-plus-tcp_SOURCE_DIR ${freertos-plus-tcp_SOURCE_DIR} PARENT_SCOPE)

add_subdirectory(coreMQTT-Agent)
set(coremqtt-agent_SOURCE_DIR ${coremqtt-agent_SOURCE_DIR} PARENT_SCOPE)

add_subdirectory(coreMQTT)
add_subdirectory(coreJSON)
add_subdirectory(coreHTTP)
add_subdirectory(mbedtls)
add_subdirectory(tinycbor)
add_subdirectory(backoff_algorithm)
add_subdirectory(ota_for_aws)
# add_subdirectory(corePKCS11)

add_library(${LIB_NAME} STATIC EXCLUDE_FROM_ALL)

target_include_directories(${LIB_NAME}
    PUBLIC
        "${aws-iot-device-sdk_SOURCE_DIR}/libraries/3rdparty/tinycbor/src"
        "${aws-iot-device-sdk_SOURCE_DIR}/libraries/aws/ota-for-aws-iot-embedded-sdk/source/include"
        "${aws-iot-device-sdk_SOURCE_DIR}/libraries/aws/ota-for-aws-iot-embedded-sdk/source/portable/os"
        "${aws-iot-device-sdk_SOURCE_DIR}/libraries/standard/corePKCS11/source/dependency/3rdparty/pkcs11"
        "${aws-iot-device-sdk_SOURCE_DIR}/libraries/standard/corePKCS11/source/include"
        "${freertos-plus-tcp_SOURCE_DIR}/include"
        "${freertos-plus-tcp_SOURCE_DIR}/portable/Compiler/GCC"
        "${coremqtt-agent_SOURCE_DIR}/source/include"
        "${PRJ_DIR}/bsp/aws_configs"
        ${AWS_CONFIG_CREDENTIALS_PATH}
        "${PRJ_DIR}/bsp/aws_libraries/3rdparty/jsmn"
        "${PRJ_DIR}/bsp/aws_libraries/3rdparty/mbedtls_config"
        "${PRJ_DIR}/bsp/aws_libraries/3rdparty/mbedtls_utils"
        "${PRJ_DIR}/bsp/aws_libraries/3rdparty/mbedtls/include"
        "${PRJ_DIR}/bsp/aws_libraries/3rdparty/pkcs11"
        "${PRJ_DIR}/bsp/aws_libraries/abstractions/backoff_algorithm/source/include"
        "${PRJ_DIR}/bsp/aws_libraries/abstractions/mqtt_agent/include"
        "${PRJ_DIR}/bsp/aws_libraries/abstractions/pkcs11/corePKCS11/source/dependency/3rdparty/mbedtls_utils"
        "${PRJ_DIR}/bsp/aws_libraries/abstractions/pkcs11/corePKCS11/source/dependency/3rdparty/pkcs11"
        "${PRJ_DIR}/bsp/aws_libraries/abstractions/pkcs11/corePKCS11/source/include"
        "${PRJ_DIR}/bsp/aws_libraries/abstractions/pkcs11/psa"
        "${PRJ_DIR}/bsp/aws_libraries/abstractions/platform/freertos/include"
        "${PRJ_DIR}/bsp/aws_libraries/abstractions/platform/freertos/include/platform"
        "${PRJ_DIR}/bsp/aws_libraries/abstractions/platform/include"
        "${PRJ_DIR}/bsp/aws_libraries/abstractions/platform/include/platform"
        "${PRJ_DIR}/bsp/aws_libraries/abstractions/platform/include/types"
        "${PRJ_DIR}/bsp/aws_libraries/abstractions/secure_sockets/include"
        "${PRJ_DIR}/bsp/aws_libraries/abstractions/transport/secure_sockets"
        "${PRJ_DIR}/bsp/aws_libraries/c_sdk/standard/common/include"
        "${PRJ_DIR}/bsp/aws_libraries/c_sdk/standard/common/include/private"
        "${PRJ_DIR}/bsp/aws_libraries/c_sdk/standard/common/include/types"
        "${PRJ_DIR}/bsp/aws_libraries/c_sdk/standard/https/include"
        "${PRJ_DIR}/bsp/aws_libraries/c_sdk/standard/https/src"
        "${PRJ_DIR}/bsp/aws_libraries/c_sdk/standard/https/src/pkcs11/corePKCS11/source/include"
        "${PRJ_DIR}/bsp/aws_libraries/c_sdk/standard/mqtt/include"
        "${PRJ_DIR}/bsp/aws_libraries/c_sdk/standard/mqtt/include/types"
        "${PRJ_DIR}/bsp/aws_libraries/c_sdk/standard/mqtt/src/private"
        "${PRJ_DIR}/bsp/aws_libraries/demos/common/http_demo_helpers"
        "${PRJ_DIR}/bsp/aws_libraries/demos/common/mqtt_demo_helpers"
        "${PRJ_DIR}/bsp/aws_libraries/demos/common/mqtt_subscription_manager"
        "${PRJ_DIR}/bsp/aws_libraries/demos/common/ota_demo_helpers"
        "${PRJ_DIR}/bsp/aws_libraries/demos/common/pkcs11_helpers"
        "${PRJ_DIR}/bsp/aws_libraries/demos/coreMQTT_Agent"
        "${PRJ_DIR}/bsp/aws_libraries/demos/dev_mode_key_provisioning/include"
        "${PRJ_DIR}/bsp/aws_libraries/demos/network_manager"
        "${PRJ_DIR}/bsp/aws_libraries/freertos_kernel/include"
        "${PRJ_DIR}/bsp/aws_libraries/freertos_kernel/portable/GCC/ARM_CM33_NTZ/non_secure"
        "${PRJ_DIR}/bsp/aws_libraries/freertos_plus/aws/ota/include"
        "${PRJ_DIR}/bsp/aws_libraries/freertos_plus/aws/ota/src"
        "${PRJ_DIR}/bsp/aws_libraries/freertos_plus/aws/ota/src/http"
        "${PRJ_DIR}/bsp/aws_libraries/freertos_plus/aws/ota/src/mqtt"
        "${PRJ_DIR}/bsp/aws_libraries/freertos_plus/standard/crypto/include"
        "${PRJ_DIR}/bsp/aws_libraries/freertos_plus/standard/freertos_plus_tcp/include"
        "${PRJ_DIR}/bsp/aws_libraries/freertos_plus/standard/freertos_plus_tcp/portable/Compiler/GCC"
        "${PRJ_DIR}/bsp/aws_libraries/freertos_plus/standard/tls/include"
        "${PRJ_DIR}/bsp/aws_libraries/logging/include"
        "${PRJ_DIR}/kws"
        "${PRJ_DIR}/kws/include"
        "${PRJ_DIR}/kws/ota/ota_pal_psa"
        "${PRJ_DIR}/kws/ota/ota_pal_psa/version"
        "${PRJ_DIR}/kws/ota/provision"
        "${TFM_PLATFORM_TARGET_DIR}/arm/mps3/an547"
        "${TFM_PLATFORM_TARGET_DIR}/arm/mps3/an547/cmsis_drivers"
        "${TFM_PLATFORM_TARGET_DIR}/arm/mps3/an547/cmsis_drivers/config"
        "${TFM_PLATFORM_TARGET_DIR}/arm/mps3/an547/device"
        "${TFM_PLATFORM_TARGET_DIR}/arm/mps3/an547/device/config"
        "${TFM_PLATFORM_TARGET_DIR}/arm/mps3/an547/device/include"
        "${TFM_PLATFORM_TARGET_DIR}/arm/mps3/an547/device/source/armclang"
        "${TFM_PLATFORM_TARGET_DIR}/arm/mps3/an547/native_drivers"
        "${TFM_PLATFORM_TARGET_DIR}/arm/mps3/an547/partition"
        "${TFM_PLATFORM_TARGET_DIR}/arm/mps3/an547/services/src"
        "${TFM_PLATFORM_TARGET_DIR}/arm/mps3/common"
)

# Add PSA OTA include
ExternalProject_Get_Property(tf-m SOURCE_DIR)
target_include_directories(${LIB_NAME}
    PUBLIC
        "${SOURCE_DIR}/interface/include"
)
target_link_libraries(${LIB_NAME}
    tfm-ns-interface
)

target_sources(${LIB_NAME}
    PRIVATE
        # PKCS 11
        "${aws-iot-device-sdk_SOURCE_DIR}/libraries/standard/corePKCS11/source/core_pkcs11.c"
        "${aws-iot-device-sdk_SOURCE_DIR}/libraries/standard/corePKCS11/source/core_pki_utils.c"

        # OTA
        "${PRJ_DIR}/kws/ota/ota_demo_core_mqtt.c"
        "${PRJ_DIR}/kws/ota/ota_pal_psa/version/application_version.c"
        "${PRJ_DIR}/kws/ota/ota_pal_psa/ota_pal.c"
        "${PRJ_DIR}/kws/ota/provision/ota_provision.c"

        # MQTT agent
        "${PRJ_DIR}/bsp/aws_libraries/abstractions/mqtt_agent/freertos_agent_message.c"
        "${PRJ_DIR}/bsp/aws_libraries/abstractions/mqtt_agent/freertos_command_pool.c"

        # Clock abstraction
        "${PRJ_DIR}/bsp/aws_libraries/abstractions/platform/freertos/iot_clock_freertos.c"


        # common
        "${PRJ_DIR}/bsp/aws_libraries/c_sdk/standard/common/iot_device_metrics.c"
        "${PRJ_DIR}/bsp/aws_libraries/c_sdk/standard/common/iot_init.c"
        "${PRJ_DIR}/bsp/aws_libraries/c_sdk/standard/common/iot_static_memory_common.c"
        "${PRJ_DIR}/bsp/aws_libraries/c_sdk/standard/common/taskpool/iot_taskpool.c"
        "${PRJ_DIR}/bsp/aws_libraries/c_sdk/standard/common/taskpool/iot_taskpool_static_memory.c"

        # HTTPS
        "${PRJ_DIR}/bsp/aws_libraries/c_sdk/standard/https/src/iot_https_client.c"
        "${PRJ_DIR}/bsp/aws_libraries/c_sdk/standard/https/src/iot_https_utils.c"

        # LOGGING
        "${PRJ_DIR}/bsp/aws_libraries/logging/iot_logging.c"
        "${PRJ_DIR}/bsp/aws_libraries/logging/iot_logging_task_dynamic_buffers.c"

        "${PRJ_DIR}/bsp/aws_libraries/abstractions/platform/freertos/iot_metrics.c"

        "${PRJ_DIR}/bsp/aws_libraries/abstractions/platform/freertos/iot_network_freertos.c"

        # PKCS11 - PSA
        "${PRJ_DIR}/bsp/aws_libraries/abstractions/pkcs11/psa/iot_pkcs11_psa.c"
        "${PRJ_DIR}/bsp/aws_libraries/abstractions/pkcs11/psa/iot_pkcs11_psa_input_format.c"
        "${PRJ_DIR}/bsp/aws_libraries/abstractions/pkcs11/psa/iot_pkcs11_psa_object_management.c"

        "${PRJ_DIR}/bsp/aws_libraries/abstractions/platform/freertos/iot_threads_freertos.c"

        # MBEDTLS UTIL
        "${PRJ_DIR}/bsp/aws_libraries/3rdparty/mbedtls_utils/mbedtls_error.c"
        "${PRJ_DIR}/bsp/aws_libraries/3rdparty/mbedtls_utils/mbedtls_utils.c"

        # Secure socket
        "${PRJ_DIR}/bsp/aws_libraries/abstractions/transport/secure_sockets/transport_secure_sockets.c"

        # Demo code
        "${PRJ_DIR}/bsp/aws_libraries/demos/common/http_demo_helpers/http_demo_utils.c"
        "${PRJ_DIR}/bsp/aws_libraries/demos/common/mqtt_demo_helpers/mqtt_demo_helpers.c"
        "${PRJ_DIR}/bsp/aws_libraries/demos/common/mqtt_subscription_manager/mqtt_subscription_manager.c"
        "${PRJ_DIR}/bsp/aws_libraries/demos/common/ota_demo_helpers/ota_application_version.c"
        "${PRJ_DIR}/bsp/aws_libraries/demos/common/pkcs11_helpers/pkcs11_helpers.c"
        "${PRJ_DIR}/bsp/aws_libraries/demos/coreMQTT_Agent/mqtt_agent_task.c"
        "${PRJ_DIR}/bsp/aws_libraries/demos/coreMQTT_Agent/simple_sub_pub_demo.c"
        "${PRJ_DIR}/bsp/aws_libraries/demos/coreMQTT_Agent/subscription_manager.c"
        "${PRJ_DIR}/bsp/aws_libraries/demos/demo_runner/aws_demo_network_addr.c"
        "${PRJ_DIR}/bsp/aws_libraries/demos/demo_runner/aws_demo.c"
        "${PRJ_DIR}/bsp/aws_libraries/demos/demo_runner/iot_demo_freertos.c"
        "${PRJ_DIR}/bsp/aws_libraries/demos/demo_runner/iot_demo_runner.c"
        "${PRJ_DIR}/bsp/aws_libraries/demos/dev_mode_key_provisioning/src/aws_dev_mode_key_provisioning.c"
        "${PRJ_DIR}/bsp/aws_libraries/demos/network_manager/aws_iot_network_manager.c"
)

target_link_libraries(${LIB_NAME}
    coreMQTT-Agent
    coreMQTT
    coreJSON
    coreHTTP
    mbedtls
    tinycbor
    ota_for_aws
    backoff_algorithm
    freertos-kernel-heap-4
    freertos-kernel
)
