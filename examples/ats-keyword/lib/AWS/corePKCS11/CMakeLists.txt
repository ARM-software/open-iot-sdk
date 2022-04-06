# Copyright (c) 2021 ARM Limited. All rights reserved.
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.18)

set(LIB_NAME corePKCS11)

include(${aws-iot-device-sdk_SOURCE_DIR}/libraries/standard/corePKCS11/pkcsFilePaths.cmake)
include(${aws-iot-device-sdk_SOURCE_DIR}/demos/logging-stack/logging.cmake)

add_library(${LIB_NAME} STATIC EXCLUDE_FROM_ALL)

# FIXME: Cleanup: dependency to Mbed TLS and Free RTOS

set(MBEDTLS_INCLUDE_PATH ${aws-iot-device-sdk_SOURCE_DIR}/libraries/3rdparty/mbedtls/include)
set(FREERTOS_TARGET_INCLUDE_PATH ${TFM_PLATFORM_TARGET_DIR}/arm/mps3/an547/device/include)
set(FREERTOS_BSP_INCLUDE_PATH ${PRJ_DIR}/bsp/freertos-platform)

target_compile_definitions(${LIB_NAME}
    PUBLIC
        -DMBEDTLS_CONFIG_FILE=<aws_mbedtls_config.h>
)

target_include_directories(${LIB_NAME}
    PUBLIC
        ${PKCS_INCLUDE_PUBLIC_DIRS}
        ${PKCS_PAL_INCLUDE_PUBLIC_DIRS}
        ${PKCS11_CONFIG_PATH}
        ${LOGGING_INCLUDE_DIRS}
        ${MBEDTLS_INCLUDE_PATH}
        ${FREERTOS_TARGET_INCLUDE_PATH}
        ${FREERTOS_BSP_INCLUDE_PATH}
        ${MBEDTLS_CONFIG_PATH}

)

target_link_libraries(${LIB_NAME}
    PUBLIC
        freertos-kernel
)

target_sources(${LIB_NAME}
    PRIVATE
        ${PKCS_SOURCES}
)
