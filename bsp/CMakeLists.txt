# Copyright (c) 2021-2023, Arm Limited and Contributors. All rights reserved.
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.18)

add_subdirectory(aws_configs)
add_subdirectory(mbedtls_config)

add_library(ts-bsp STATIC)

target_include_directories(ts-bsp
    PUBLIC
        ${CMAKE_SOURCE_DIR}/bsp/platform
        $<$<STREQUAL:${TS_TARGET},Corstone-300>:${CMAKE_CURRENT_SOURCE_DIR}/ethos-u55/an552>
        $<$<STREQUAL:${TS_TARGET},Corstone-310>:${CMAKE_CURRENT_SOURCE_DIR}/ethos-u55/an555>
)

target_sources(ts-bsp
    PRIVATE
        "${CMAKE_SOURCE_DIR}/bsp/platform/application_helpers.c"
        "${CMAKE_SOURCE_DIR}/bsp/platform/print_log.c"
)

target_link_libraries(ts-bsp
    PRIVATE
        cmsis-rtos-api
        tfm-ns-interface
        iotsdk-cmsis-core-device
        aws-configs
)

target_compile_definitions(ts-bsp
    PRIVATE
        BL2
        $<$<STREQUAL:${CMAKE_SYSTEM_PROCESSOR},cortex-m55>:CPU_CORTEX_M55=1>
        $<$<STREQUAL:${CMAKE_SYSTEM_PROCESSOR},cortex-m85>:CPU_CORTEX_M85=1>
        DAUTH_CHIP_DEFAULT
        LIBRARY_LOG_LEVEL=3
        MCUBOOT_DATA_SHARING=1
        MCUBOOT_IMAGE_NUMBER=2
        MCUBOOT_SIGN_RSA
        MCUBOOT_SIGN_RSA_LEN=2048
        MPS3_PLATFORM=1
        PLATFORM_HAL=1
        PLATFORM_DEFAULT_UART_STDOUT=ON
        TFM_PARTITION_CRYPTO
        TFM_PARTITION_FIRMWARE_UPDATE
        TFM_PARTITION_INITIAL_ATTESTATION
        TFM_PARTITION_INTERNAL_TRUSTED_STORAGE
        TFM_PARTITION_LOG_LEVEL=TFM_PARTITION_LOG_LEVEL_DEBUG
        TFM_PARTITION_PLATFORM
        TFM_PARTITION_PROTECTED_STORAGE
        TFM_SPM_LOG_LEVEL=0
        TFM_SYSTEM_FP=2
        TFM_SYSTEM_FP_NS=2
    PUBLIC
        $<$<STREQUAL:${RTOS},FREERTOS>:USE_FREERTOS>
        $<$<STREQUAL:${RTOS},RTX>:USE_RTX>
        $<$<STREQUAL:${RTOS},THREADX>:USE_THREADX>
)

set(corstone-300-cmsis-pack
    "ARM::V2M_MPS3_SSE_300_BSP@1.3.0"
    "ARM::CMSIS@5.9.0"
    "ARM::CMSIS-Driver@2.7.2"
)

set(corstone-300-cmsis-pack-components
    "ARM::Device:Definition"
    "ARM::Device:Startup&Baremetal"
    "CMSIS Driver:USART"
    "CMSIS Driver:Ethernet:ETH_LAN91C111"
    "ARM::Native Driver:IO"
    "ARM::Native Driver:UART"
)

set(corstone-310-cmsis-pack
    "ARM::V2M_MPS3_SSE_310_BSP@1.1.0"
    "ARM::CMSIS@5.9.0"
    "ARM::CMSIS-Driver@2.7.2"
)

set(corstone-310-cmsis-pack-components
    "ARM::Device:Definition"
    "ARM::Device:Startup&Baremetal"
    "CMSIS Driver:USART"
    "CMSIS Driver:Ethernet:ETH_LAN91C111"
    "ARM::Native Driver:IO"
    "ARM::Native Driver:UART"
)

set(ENV{CMSIS_PACK_ROOT} ${CMAKE_BINARY_DIR}/cmsis-packs)

if (${CMSIS_PACK_PLATFORM_DEVICE_NAME} STREQUAL "SSE-300-MPS3")
    set(cmsis_packs corstone-300-cmsis-pack)
    set(cmsis_pack_components corstone-300-cmsis-pack-components)
    set(cmsis_pack_device_name "SSE-300-MPS3")
elseif(${CMSIS_PACK_PLATFORM_DEVICE_NAME} STREQUAL "SSE-310-MPS3_FVP")
    set(cmsis_packs corstone-310-cmsis-pack)
    set(cmsis_pack_components corstone-310-cmsis-pack-components)
    set(cmsis_pack_device_name "SSE-310-MPS3")
endif()

if(CMAKE_C_COMPILER_ID STREQUAL "ARMClang")
    set(cmsis_pack_compiler "AC6")
elseif(CMAKE_C_COMPILER_ID STREQUAL "GNU" AND ${CMSIS_PACK_PLATFORM_DEVICE_NAME} STREQUAL "SSE-310-MPS3_FVP")
    set(cmsis_pack_compiler "GCC")
elseif(CMAKE_C_COMPILER_ID STREQUAL "GNU" AND ${CMSIS_PACK_PLATFORM_DEVICE_NAME} STREQUAL "SSE-300-MPS3")
# Note: ARM::V2M_MPS3_SSE_300_BSP CMSIS Pack does not support GCC compiler
# and assume it will only be used with Armclang compiler and set it as requirement.
# Our examples do not use compiler dependent parts from the packs
    message(WARNING "SSE-300 Pack does not support GCC compiler, AC6 will be used as the pack compiler")
    set(cmsis_pack_compiler "AC6")
else()
    message(FATAL_ERROR "Invalid compiler chosen, please use either `ARMClang` or `GNU` compiler")
endif()

add_cmsis_library(
  cmsis-pack-platform
  ${cmsis_pack_device_name}
  ${cmsis_pack_compiler}
  ${cmsis_packs}
  ${cmsis_pack_components}
)

if(CMAKE_C_COMPILER_ID STREQUAL "GNU")
    # Reverse the bit order for ETH_LAN91C111 Driver
    target_compile_definitions(cmsis-pack-platform
        PRIVATE
            __rbit=__RBIT
    )
endif()

# The startup files that are located in the device CMSIS pack depends on
# iotsdk-cmsis-core-device so it has to be linked to cmsis-pack-platform
target_link_libraries(cmsis-pack-platform
    PUBLIC
        iotsdk-cmsis-core-device
        lwip-cmsis-port
)

# The cmsis-pack-platform library contains the system specific header files
# so it has to be linked to iotsdk-cmsis-core-device
target_link_libraries(iotsdk-cmsis-core-device
    INTERFACE
        cmsis-pack-platform
)

# Board support package drivers library

add_library(bsp_lib STATIC
    "${CMAKE_SOURCE_DIR}/bsp/platform/mps3_leds.c"
    $<$<STREQUAL:${AUDIO},VSI>:${CMAKE_SOURCE_DIR}/bsp/platform/sai_driver.c>
)

target_include_directories(bsp_lib
    PUBLIC
        ${CMAKE_SOURCE_DIR}/bsp/platform
)

add_library(ts-retarget INTERFACE)

target_sources(ts-retarget
    INTERFACE
        $<$<STREQUAL:${CMAKE_C_COMPILER_ID},GNU>:${CMAKE_SOURCE_DIR}/bsp/platform/gcc_retarget.c>
        $<$<STREQUAL:${CMAKE_C_COMPILER_ID},ARMClang>:${CMAKE_SOURCE_DIR}/bsp/platform/armclang_retarget.c>
)

target_link_libraries(ts-retarget
    INTERFACE
        cmsis-rtos-api
)

target_compile_definitions(bsp_lib
    PRIVATE
    $<$<STREQUAL:${CMAKE_SYSTEM_PROCESSOR},cortex-m55>:CMSIS_device_header="${CMAKE_BINARY_DIR}/_deps/cmsis-5-src/Device/ARM/ARMCM55/Include/ARMCM55.h">
    $<$<STREQUAL:${CMAKE_SYSTEM_PROCESSOR},cortex-m85>:CMSIS_device_header="${CMAKE_BINARY_DIR}/_deps/cmsis-5-src/Device/ARM/ARMCM85/Include/ARMCM85.h">
)

target_link_libraries(bsp_lib
    PRIVATE
        cmsis-pack-platform
)

target_link_libraries(ts-bsp
    PRIVATE
        bsp_lib
)

# OS alloc wrapper
add_library(heap-alloc-wrapper STATIC)
target_sources(heap-alloc-wrapper
    PRIVATE
        cmsis-config/heap-alloc.c
)


# Add credentials library
add_library(cloud-credentials INTERFACE)

target_include_directories(cloud-credentials
    INTERFACE
        ${APP_CONFIG_CREDENTIALS_PATH}
)

# Add Startup library
add_library(startup
    $<$<STREQUAL:${TS_TARGET},Corstone-300>:${TFM_PLATFORM}/device/source/startup_an552.c>
    $<$<STREQUAL:${TS_TARGET},Corstone-310>:tf_m_targets/arm/mps3/corstone310/common/device/source/startup.c>
)

# This is set as the Corstone-310 Startup file sets values in the vector table depending on if the user is using a Corstone-310 FVP or FPGA
target_compile_definitions(startup PRIVATE CORSTONE310_FVP)

if(${TS_TARGET} STREQUAL "Corstone-300")
    set(ld_linker_script ${CMAKE_CURRENT_LIST_DIR}/an552_ns.ld)
    set(armlink_linker_script ${CMAKE_CURRENT_LIST_DIR}/an552_ns.sct)
elseif(${TS_TARGET} STREQUAL "Corstone-310")
    set(ld_linker_script ${CMAKE_CURRENT_LIST_DIR}/an555_ns.ld)
    set(armlink_linker_script ${CMAKE_CURRENT_LIST_DIR}/an555_ns.sct)
endif()


if(CMAKE_C_COMPILER_ID STREQUAL "GNU")
    target_link_options(startup INTERFACE -T ${ld_linker_script})
elseif(CMAKE_C_COMPILER_ID STREQUAL "ARMClang")
    target_link_options(startup INTERFACE --scatter=${armlink_linker_script})
endif()

FetchContent_GetProperties(trusted-firmware-m)
target_include_directories(startup
    PRIVATE
        "${trusted-firmware-m_SOURCE_DIR}/platform/include"
        $<$<STREQUAL:${TS_TARGET},Corstone-300>:${TFM_PLATFORM}/device/include>
        $<$<STREQUAL:${TS_TARGET},Corstone-300>:${TFM_PLATFORM}/partition>
        $<$<STREQUAL:${TS_TARGET},Corstone-310>:${trusted-firmware-m_SOURCE_DIR}/platform/ext/target/${TFM_PLATFORM}/../common/device/include>
        $<$<STREQUAL:${TS_TARGET},Corstone-310>:${trusted-firmware-m_SOURCE_DIR}/platform/ext/target/${TFM_PLATFORM}/../common/partition>
)

target_link_libraries(startup
    PUBLIC
        tfm-ns-interface
        cmsis-pack-platform
)
