# Copyright (c) 2021-2022, Arm Limited and Contributors. All rights reserved.
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.18)

add_subdirectory(aws_configs)
add_subdirectory(mbedtls_config)

add_library(ts-bsp STATIC)

target_include_directories(ts-bsp
    PUBLIC
        ${PRJ_DIR}/bsp/platform
        $<$<STREQUAL:${TS_TARGET},Corstone-300>:${CMAKE_CURRENT_SOURCE_DIR}/ethos-u55/an547>
        $<$<STREQUAL:${TS_TARGET},Corstone-310>:${CMAKE_CURRENT_SOURCE_DIR}/ethos-u55/an555>
)

target_sources(ts-bsp
    PRIVATE
        "${PRJ_DIR}/bsp/platform/application_helpers.c"
        "${PRJ_DIR}/bsp/platform/print_log.c"
)

target_link_libraries(ts-bsp
    PRIVATE
        cmsis-rtos-api
        mcu-driver-hal
        tfm-ns-interface
    INTERFACE
        aws-configs
)

target_compile_definitions(ts-bsp
    PRIVATE
        -DBL2
        -D$<$<STREQUAL:${CMAKE_SYSTEM_PROCESSOR},cortex-m55>:CPU_CORTEX_M55=1>
        -D$<$<STREQUAL:${CMAKE_SYSTEM_PROCESSOR},cortex-m85>:CPU_CORTEX_M85=1>
        -DDAUTH_CHIP_DEFAULT
        -DLIBRARY_LOG_LEVEL=3
        -DMCUBOOT_DATA_SHARING=1
        -DMCUBOOT_IMAGE_NUMBER=2
        -DMCUBOOT_SIGN_RSA
        -DMCUBOOT_SIGN_RSA_LEN=2048
        -DMPS3_PLATFORM=1
        -DPLATFORM_HAL=1
        -DPLATFORM_DEFAULT_UART_STDOUT=ON
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
        "${PRJ_DIR}/bsp/platform/bsp_serial.c"
)

target_include_directories(bsp_serial
    PUBLIC
        ${PRJ_DIR}/bsp/platform
)

target_link_libraries(bsp_serial
    PRIVATE
        mcu-driver-hal
)

target_link_libraries(ts-bsp
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


# Add credentials library
add_library(cloud-credentials INTERFACE)

target_include_directories(cloud-credentials
    INTERFACE
        ${APP_CONFIG_CREDENTIALS_PATH}
)

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
