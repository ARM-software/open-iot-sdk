# Copyright (c) 2021 ARM Limited. All rights reserved.
# SPDX-License-Identifier: Apache-2.0

set(LIB_NAME FreeRTOS-Plus-TCP)

include(FetchContent)
FetchContent_Declare(
    FreeRTOS-Plus-TCP
    GIT_REPOSITORY https://github.com/FreeRTOS/FreeRTOS-Plus-TCP
    GIT_TAG        a1176444c9620706306840e21a8880fc519817dc
    GIT_PROGRESS   ON
)
FetchContent_Populate(FreeRTOS-Plus-TCP)

set(freertos-plus-tcp_SOURCE_DIR ${freertos-plus-tcp_SOURCE_DIR} PARENT_SCOPE)

include(${aws-iot-device-sdk_SOURCE_DIR}/libraries/standard/coreJSON/jsonFilePaths.cmake)

add_library(${LIB_NAME} STATIC EXCLUDE_FROM_ALL
    "${freertos-plus-tcp_SOURCE_DIR}/FreeRTOS_ARP.c"
    "${freertos-plus-tcp_SOURCE_DIR}/FreeRTOS_DHCP.c"
    "${freertos-plus-tcp_SOURCE_DIR}/FreeRTOS_DNS.c"
    "${freertos-plus-tcp_SOURCE_DIR}/FreeRTOS_IP.c"
    "${freertos-plus-tcp_SOURCE_DIR}/FreeRTOS_Sockets.c"
    "${freertos-plus-tcp_SOURCE_DIR}/FreeRTOS_Stream_Buffer.c"
    "${freertos-plus-tcp_SOURCE_DIR}/FreeRTOS_TCP_IP.c"
    "${freertos-plus-tcp_SOURCE_DIR}/FreeRTOS_TCP_WIN.c"
    "${freertos-plus-tcp_SOURCE_DIR}/FreeRTOS_UDP_IP.c"
    "${freertos-plus-tcp_SOURCE_DIR}/portable/BufferManagement/BufferAllocation_2.c"
)

target_include_directories(${LIB_NAME}
    PUBLIC
        ${PRJ_DIR}/bsp/aws_configs
        ${freertos-plus-tcp_SOURCE_DIR}/include
        ${freertos-plus-tcp_SOURCE_DIR}/portable/Compiler/GCC
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
)

target_link_libraries(${LIB_NAME}
    PUBLIC
        freertos-kernel
)
