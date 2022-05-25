# Copyright (c) 2021-2022, Arm Limited and Contributors. All rights reserved.
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.18)

set(LIB_NAME lib-bsp)

add_library(${LIB_NAME} STATIC)

target_include_directories(${LIB_NAME}
    PRIVATE
        ${freertos-plus-tcp_SOURCE_DIR}/include
        ${freertos-plus-tcp_SOURCE_DIR}/portable/Compiler/GCC
        ${aws-iot-device-sdk_SOURCE_DIR}/libraries/standard/corePKCS11/source/dependency/3rdparty/mbedtls_utils
        ${aws-iot-device-sdk_SOURCE_DIR}/libraries/standard/corePKCS11/source/dependency/3rdparty/pkcs11
        ${aws-iot-device-sdk_SOURCE_DIR}/libraries/standard/corePKCS11/source/include
        ${PRJ_DIR}/kws
        ${PRJ_DIR}/kws/include
        ${PRJ_DIR}/kws/ota/ota_pal_psa
        ${PRJ_DIR}/kws/ota/ota_pal_psa/version
        ${PRJ_DIR}/kws/ota/provision
        ${PRJ_DIR}/lib/tf-m
        ${PRJ_DIR}/lib/tf-m/interface/include
        ${PRJ_DIR}/lib/tf-m/interface/include/psa
        ${PRJ_DIR}/lib/tf-m/platform/ext
        ${PRJ_DIR}/lib/tf-m/platform/ext/cmsis
        ${PRJ_DIR}/lib/tf-m/platform/ext/common
        ${PRJ_DIR}/lib/tf-m/platform/ext/driver
        ${PRJ_DIR}/lib/tf-m/platform/include
        ${PRJ_DIR}/lib/tfm_test/ns_interface
    PUBLIC
        ${TFM_PLATFORM_TARGET_DIR}/arm/mps3/an547
        ${TFM_PLATFORM_TARGET_DIR}/arm/mps3/an547/cmsis_drivers
        ${TFM_PLATFORM_TARGET_DIR}/arm/mps3/an547/cmsis_drivers/config
        ${TFM_PLATFORM_TARGET_DIR}/arm/mps3/an547/device
        ${TFM_PLATFORM_TARGET_DIR}/arm/mps3/an547/device/config
        ${TFM_PLATFORM_TARGET_DIR}/arm/mps3/an547/device/include
        ${TFM_PLATFORM_TARGET_DIR}/arm/mps3/an547/device/source/armclang
        ${TFM_PLATFORM_TARGET_DIR}/arm/mps3/an547/native_drivers
        ${TFM_PLATFORM_TARGET_DIR}/arm/mps3/an547/partition
        ${TFM_PLATFORM_TARGET_DIR}/arm/mps3/an547/services/src
        ${TFM_PLATFORM_TARGET_DIR}/arm/mps3/common
        ${PRJ_DIR}/bsp/aws_configs
        ${AWS_CONFIG_CREDENTIALS_PATH}
        ${PRJ_DIR}/bsp/aws_libraries/3rdparty/jsmn
        ${PRJ_DIR}/bsp/aws_libraries/3rdparty/mbedtls_config
        ${PRJ_DIR}/bsp/aws_libraries/3rdparty/mbedtls_utils
        ${PRJ_DIR}/bsp/aws_libraries/3rdparty/mbedtls/include
        ${PRJ_DIR}/bsp/aws_libraries/3rdparty/pkcs11
        ${PRJ_DIR}/bsp/aws_libraries/abstractions/backoff_algorithm/source/include
        ${PRJ_DIR}/bsp/aws_libraries/abstractions/mqtt_agent/include
        ${PRJ_DIR}/bsp/aws_libraries/abstractions/pkcs11/psa
        ${PRJ_DIR}/bsp/aws_libraries/abstractions/platform/freertos/include
        ${PRJ_DIR}/bsp/aws_libraries/abstractions/platform/freertos/include/platform
        ${PRJ_DIR}/bsp/aws_libraries/abstractions/platform/include
        ${PRJ_DIR}/bsp/aws_libraries/abstractions/platform/include/platform
        ${PRJ_DIR}/bsp/aws_libraries/abstractions/platform/include/types
        ${PRJ_DIR}/bsp/aws_libraries/abstractions/secure_sockets/include
        ${PRJ_DIR}/bsp/aws_libraries/abstractions/transport/secure_sockets
        ${PRJ_DIR}/bsp/aws_libraries/c_sdk/standard/common/include
        ${PRJ_DIR}/bsp/aws_libraries/c_sdk/standard/common/include/private
        ${PRJ_DIR}/bsp/aws_libraries/c_sdk/standard/common/include/types
        ${PRJ_DIR}/bsp/aws_libraries/c_sdk/standard/https/include
        ${PRJ_DIR}/bsp/aws_libraries/c_sdk/standard/mqtt/include
        ${PRJ_DIR}/bsp/aws_libraries/c_sdk/standard/mqtt/include/types
        ${PRJ_DIR}/bsp/aws_libraries/c_sdk/standard/mqtt/src/private
        ${PRJ_DIR}/bsp/aws_libraries/coreHTTP/source/dependency/3rdparty/http_parser
        ${PRJ_DIR}/bsp/aws_libraries/coreHTTP/source/include
        ${PRJ_DIR}/bsp/aws_libraries/coreHTTP/source/interface
        ${PRJ_DIR}/bsp/aws_libraries/coreMQTT-Agent/source/include
        ${PRJ_DIR}/bsp/aws_libraries/coreMQTT/source/include
        ${PRJ_DIR}/bsp/aws_libraries/coreMQTT/source/interface
        ${PRJ_DIR}/bsp/aws_libraries/demos/common/http_demo_helpers
        ${PRJ_DIR}/bsp/aws_libraries/demos/common/mqtt_demo_helpers
        ${PRJ_DIR}/bsp/aws_libraries/demos/common/mqtt_subscription_manager
        ${PRJ_DIR}/bsp/aws_libraries/demos/common/ota_demo_helpers
        ${PRJ_DIR}/bsp/aws_libraries/demos/common/pkcs11_helpers
        ${PRJ_DIR}/bsp/aws_libraries/demos/coreMQTT_Agent
        ${PRJ_DIR}/bsp/aws_libraries/demos/dev_mode_key_provisioning/include
        ${PRJ_DIR}/bsp/aws_libraries/demos/network_manager
        ${PRJ_DIR}/bsp/aws_libraries/freertos_plus/aws/ota/include
        ${PRJ_DIR}/bsp/aws_libraries/freertos_plus/aws/ota/src
        ${PRJ_DIR}/bsp/aws_libraries/freertos_plus/aws/ota/src/http
        ${PRJ_DIR}/bsp/aws_libraries/freertos_plus/aws/ota/src/mqtt
        ${PRJ_DIR}/bsp/aws_libraries/freertos_plus/standard/crypto/include
        ${PRJ_DIR}/bsp/aws_libraries/freertos_plus/standard/tls/include
        ${PRJ_DIR}/bsp/aws_libraries/logging/include
        ${PRJ_DIR}/bsp/aws_libraries/3rdparty/lwip_osal/include
        ${PRJ_DIR}/bsp/freertos-config
        ${PRJ_DIR}/bsp/freertos-platform
        $<$<STREQUAL:${TS_TARGET},Corstone-300>:${CMAKE_CURRENT_SOURCE_DIR}/ethos-u55/an547>
        $<$<STREQUAL:${TS_TARGET},Corstone-310>:${CMAKE_CURRENT_SOURCE_DIR}/ethos-u55/an555>
)

target_sources(${LIB_NAME}
    PRIVATE
        "${PRJ_DIR}/bsp/freertos-platform/application_helpers.c"
        "${PRJ_DIR}/bsp/freertos-platform/print_log.c"
)

target_link_libraries(${LIB_NAME}
    PRIVATE
        cmsis-rtos-api
        mcu-driver-hal
        tfm-ns-interface
)

target_compile_definitions(${LIB_NAME}
    PRIVATE
        -DACTIVATION_BUF_SRAM_SZ=0x00400000
        -DACTIVATION_BUF_SZ=0x00400000
        -DARM_MATH_DSP
        -DARM_MATH_LOOPUNROLL
        -DARM_MATH_MVEI
        -DARM_NPU=1
        -DBL2
        -DCONFIG_KWS_SCORE_THRESHOLD=0.8
        -D$<$<STREQUAL:${CMAKE_SYSTEM_PROCESSOR},cortex-m55>:CPU_CORTEX_M55=1>
        -D$<$<STREQUAL:${CMAKE_SYSTEM_PROCESSOR},cortex-m85>:CPU_CORTEX_M85=1>
        -DDAUTH_CHIP_DEFAULT
        -DLIBRARY_LOG_LEVEL=3
        -DMCUBOOT_DATA_SHARING=1
        -DMCUBOOT_IMAGE_NUMBER=2
        -DMCUBOOT_SIGN_RSA
        -DMCUBOOT_SIGN_RSA_LEN=2048
        -DMPS3_PLATFORM=1
        -DTFM_PARTITION_AUDIT_LOG
        -DTFM_PARTITION_CRYPTO
        -DTFM_PARTITION_FIRMWARE_UPDATE
        -DTFM_PARTITION_INITIAL_ATTESTATION
        -DTFM_PARTITION_INTERNAL_TRUSTED_STORAGE
        -DTFM_PARTITION_LOG_LEVEL=TFM_PARTITION_LOG_LEVEL_DEBUG
        -DTFM_PARTITION_PLATFORM
        -DTFM_PARTITION_PROTECTED_STORAGE
        -DTFM_SPM_LOG_LEVEL=0
        -DTFM_SYSTEM_FP=2
        -DTFM_SYSTEM_FP_NS=2
)

# BSP serial library

add_library(bsp_serial STATIC)

target_sources(bsp_serial
    PRIVATE
        "${PRJ_DIR}/bsp/freertos-platform/bsp_serial.c"
)

target_include_directories(bsp_serial
    PUBLIC
        ${PRJ_DIR}/bsp/freertos-platform
)

target_link_libraries(bsp_serial
    PRIVATE
        mcu-driver-hal
)

target_link_libraries(${LIB_NAME}
    PRIVATE
        bsp_serial
)

# OS alloc wrapper
if (TARGET cmsis-rtx)
    add_library(cmsis-rtx-freertos-alloc-wrapper STATIC)
    target_sources(cmsis-rtx-freertos-alloc-wrapper
        PRIVATE
            cmsis-config/rtx-alloc.c
    )
endif()


# Add Startup library
add_library(startup
    $<$<STREQUAL:${TS_TARGET},Corstone-300>:${TFM_PLATFORM}/device/source/startup_an547_ns.c>
    $<$<STREQUAL:${TS_TARGET},Corstone-310>:${TFM_PLATFORM}/device/source/startup_corstone_polaris_ns.c>
)

set(link_definitions_file ${CMAKE_CURRENT_BINARY_DIR}/link_definitions.txt)

if (${TS_TARGET} STREQUAL "Corstone-300")
    mdh_generate_definitions_for_linker(mdh-arm-corstone-300-startup ${link_definitions_file})
    mdh_set_linker_script(startup "${PRJ_DIR}/bsp/an547_ns.sct" ${link_definitions_file})
elseif(${TS_TARGET} STREQUAL "Corstone-310")
    mdh_generate_definitions_for_linker(mdh-arm-corstone-310-startup ${link_definitions_file})
    mdh_set_linker_script(startup "${PRJ_DIR}/bsp/an555_ns.sct" ${link_definitions_file})
endif()

FetchContent_GetProperties(tf-m)
target_include_directories(startup
    PRIVATE
        "${tf-m_SOURCE_DIR}/platform/include"
)

target_link_libraries(startup
    PUBLIC
        tfm-ns-interface
        mcu-driver-hal
)
