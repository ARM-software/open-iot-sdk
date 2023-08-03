# Copyright (c) 2021-2023, Arm Limited and Contributors. All rights reserved.
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.21)

include(ExternalProject)
include(FetchContent)

# Declare the target of the Total Solutions project
set(TS_TARGET "Corstone-300" CACHE STRING "Hardware target of the Total Solution")

# Declare the default inference engine
set(INFERENCE_ENGINE "ETHOS" CACHE STRING "Inference engine used by total solution")

# Declare where audio data comes from
set(AUDIO "VSI" CACHE STRING "Source of audio data (VSI or ROM) used by Total Solutions")

# CMake processes toolchain file during first call to project(), so it needs to exist first
FetchContent_Declare(
    iotsdk-toolchains
    GIT_REPOSITORY  https://git.gitlab.arm.com/iot/open-iot-sdk/toolchain.git
    GIT_TAG         d600b9dc9aca55c8428b63b9b8eff1ff97743382
    SOURCE_DIR      ${CMAKE_BINARY_DIR}/toolchains
)
FetchContent_MakeAvailable(iotsdk-toolchains)

# CMake processes platform files during the first call to project().
# Platform files are located inside https://gitlab.arm.com/iot/open-iot-sdk/libraries/toolchain/-/tree/main/Modules/Platform
# CMake decides which platform files to process based on CMAKE_SYSTEM_PROCESSOR, so its value
# needs to be set before the first call to project().
if(${TS_TARGET} STREQUAL "Corstone-300")
    set(CMAKE_SYSTEM_PROCESSOR cortex-m55)
    set(CMSIS_PACK_PLATFORM_DEVICE_NAME SSE-300-MPS3)
elseif(${TS_TARGET} STREQUAL "Corstone-310")
    set(CMAKE_SYSTEM_PROCESSOR cortex-m85)
    set(CMSIS_PACK_PLATFORM_DEVICE_NAME SSE-310-MPS3_FVP)
else()
    message(FATAL_ERROR "Invalid TS_TARGET (${TS_TARGET}) set. It should be Corstone-300 or Corstone-310")
endif()

# Start of the project
project(keyword LANGUAGES C CXX)

# Configure target
# - Declare relative path to TFM target
if (${TS_TARGET} STREQUAL "Corstone-300")
    set(TFM_PLATFORM "${CMAKE_CURRENT_LIST_DIR}/bsp/tf_m_targets/arm/mps3/an552")
    set(ML_TARGET_SUBSYSTEM "sse-300")
    set(ETHOS_U_NPU_CONFIG_ID "H128")
    set(ETHOSU_TARGET_NPU_CONFIG "ethos-u55-128")
elseif(${TS_TARGET} STREQUAL "Corstone-310")
    set(TFM_PLATFORM "arm/mps3/corstone310/fvp")
    set(ML_TARGET_SUBSYSTEM "sse-310")
    set(ETHOS_U_NPU_CONFIG_ID "H256")
    set(ETHOSU_TARGET_NPU_CONFIG "ethos-u55-256")
endif()

# Set build type to prevent MCU Driver HAL to set it to Develop by default which is not correct
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "Release" CACHE STRING "The build type" FORCE)
endif()

set(FETCHCONTENT_QUIET OFF)

# Declare the Open IoT SDK
FetchContent_Declare(
  open_iot_sdk
  GIT_REPOSITORY https://git.gitlab.arm.com/iot/open-iot-sdk/sdk.git
  GIT_TAG        b3ccabab727dd9e936512a0009ab59e3265c8567
)

# Select Open IoT SDK components

set(IOTSDK_FETCH_LIST
    trusted-firmware-m
    ml-embedded-evaluation-kit
    cmsis-5
    cmsis-pack-utils
    mbedtls
    lwip
    cmsis-sockets-api
    coremqtt-agent
    mbed-atomic
    mbed-critical
)

if(${RTOS} STREQUAL "FREERTOS")
    list(APPEND IOTSDK_FETCH_LIST cmsis-freertos)
elseif(${RTOS} STREQUAL "RTX")
    list(APPEND IOTSDK_FETCH_LIST cmsis-5)
elseif(${RTOS} STREQUAL "THREADX")
    list(APPEND IOTSDK_FETCH_LIST threadx threadx-cdi-port)
    # ThreadX (if enabled) will use TX_USER_FILE to locate its configuration file
    # Note that this must be set before the IoT SDK is added
    set(TX_USER_FILE "${CMAKE_SOURCE_DIR}/bsp/threadx-config/tx_user.h")
endif()

if(${CLOUD_CLIENT} STREQUAL "AWS")
    list(APPEND IOTSDK_FETCH_LIST aws-iot-device-sdk-embedded-c)
elseif(${CLOUD_CLIENT} STREQUAL "AZURE")
    list(APPEND IOTSDK_FETCH_LIST azure-iot-sdk-c)
elseif(${CLOUD_CLIENT} STREQUAL "AZURE_NETXDUO")
    list(APPEND IOTSDK_FETCH_LIST netxduo create-adu-import-manifest)
    set(NXD_ENABLE_AZURE_IOT ON CACHE BOOL "")
    set(NXD_ENABLE_AZURE_IOT_ADU_PSA ON CACHE BOOL "")
    set(NXD_AZURE_IOT_PSA_LIBRARY tfm-ns-interface CACHE STRING "")
    set(NXD_ENABLE_FILE_SERVERS OFF CACHE BOOL "")
    # NetX Duo (if enabled) will use NX_USER_FILE to locate its configuration file
    # Note that this must be set before the IoT SDK is added
    set(NX_USER_FILE "${CMAKE_SOURCE_DIR}/bsp/netxduo-config/nx_user.h")
    # By default, NetX Duo changes WARNINGS_AS_ERRORS to ON
    set(WARNINGS_AS_ERRORS OFF)
endif()


# Configure TrustedFirmware-M
set(TFM_CMAKE_ARGS
    -DTFM_PLATFORM=${TFM_PLATFORM}
    -DPROJECT_CONFIG_HEADER_FILE=${CMAKE_CURRENT_SOURCE_DIR}/bsp/tfm-config/project_config.h
    -DCONFIG_TFM_ENABLE_CP10CP11=ON
    -DTFM_EXCEPTION_INFO_DUMP=ON
    -DNS=ON
    -DPLATFORM_DEFAULT_UART_STDOUT=ON
    -DPLATFORM_HAS_FIRMWARE_UPDATE_SUPPORT=ON
    -DMCUBOOT_DATA_SHARING=ON
    -DMCUBOOT_SIGNATURE_KEY_LEN=2048
    -DMCUBOOT_LOG_LEVEL=INFO
    -DTFM_SPM_LOG_LEVEL=TFM_SPM_LOG_LEVEL_DEBUG
    -DTFM_PARTITION_CRYPTO=ON
    -DTFM_PARTITION_FIRMWARE_UPDATE=ON
    -DTFM_PARTITION_INITIAL_ATTESTATION=ON
    -DTFM_PARTITION_INTERNAL_TRUSTED_STORAGE=ON
    -DTFM_PARTITION_PLATFORM=ON
    -DTFM_PARTITION_PROTECTED_STORAGE=ON
    -DCONFIG_TFM_HALT_ON_CORE_PANIC=ON
    -DTFM_PARTITION_LOG_LEVEL=TFM_PARTITION_LOG_LEVEL_INFO
)
if (${TS_TARGET} STREQUAL "Corstone-310")
    set(TFM_PATCH_COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_LIST_DIR}/bsp/config_tfm_target.h platform/ext/target/${TFM_PLATFORM}/config_tfm_target.h)
endif()

# Extra arguments for ML
set(ML_CMAKE_ARGS
    -DTARGET_SUBSYSTEM=${ML_TARGET_SUBSYSTEM}
    -DETHOS_U_NPU_CONFIG_ID=${ETHOS_U_NPU_CONFIG_ID}
    -DETHOSU_TARGET_NPU_CONFIG=${ETHOSU_TARGET_NPU_CONFIG}
)
if (${INFERENCE_ENGINE} STREQUAL "ETHOS")
    list(APPEND ML_CMAKE_ARGS -DETHOS_U_NPU_ENABLED=ON)
else()
    list(APPEND ML_CMAKE_ARGS -DETHOS_U_NPU_ENABLED=OFF)
endif()

set(ML_TARGETS cmsis-dsp tensorflow_build kws asr)
if(${TS_TARGET} STREQUAL "Corstone-300" AND ${INFERENCE_ENGINE} STREQUAL "ETHOS")
    list(APPEND ML_TARGETS timing_adapter)
endif()

# Apply ML patch to use tiny asr model
find_package(Git)
set(ML_PATCH_COMMAND ${GIT_EXECUTABLE} apply "${CMAKE_SOURCE_DIR}/lib/ml-kit/ml-embedded-evaluation-kit.patch" || true)

# Set library to std
set(MBED_C_LIB "std")

# Make the Open IoT SDK available
FetchContent_MakeAvailable(open_iot_sdk)

# Fetch project_options
FetchContent_Declare(
    project_options
    GIT_REPOSITORY  https://github.com/cpp-best-practices/project_options.git
    GIT_TAG         999c55957974a0057d198f51c20c2e02a5c9014c
)
FetchContent_MakeAvailable(project_options)
include(${project_options_SOURCE_DIR}/Index.cmake)
include(${project_options_SOURCE_DIR}/src/DynamicProjectOptions.cmake)

include(${open_iot_sdk_SOURCE_DIR}/cmake/AddToolchainWarningFlags.cmake)
set(GCC_WARNINGS ${IOTSDK_WARNING_FLAGS} ${IOTSDK_WARNING_AS_ERRORS_FLAGS})
set(CLANG_WARNINGS ${GCC_WARNINGS})

dynamic_project_options(
    CLANG_WARNINGS
    ${CLANG_WARNINGS}
    GCC_WARNINGS
    ${GCC_WARNINGS}
)

# get paths for the ml kit

FetchContent_GetProperties(ml-embedded-evaluation-kit)
set(ml-embedded-evaluation-kit_GENERATED_DIR "${ml-embedded-evaluation-kit_BINARY_DIR}/generated")
set(ml-embedded-evaluation-kit_LIB_DIR ${ml-embedded-evaluation-kit_BINARY_DIR}/lib)



# Path to credentials
set(APP_CONFIG_CREDENTIALS_PATH "${CMAKE_SOURCE_DIR}/bsp/default_credentials" CACHE STRING "Path to credentials")

# Include module path from Open IoT SDK
list(APPEND CMAKE_MODULE_PATH ${open_iot_sdk_SOURCE_DIR}/components/trusted-firmware-m)
list(APPEND CMAKE_MODULE_PATH ${open_iot_sdk_SOURCE_DIR}/cmake)

# Configuration of executable
set(EXE_SUFFIX ".axf")
set(CMAKE_EXECUTABLE_SUFFIX ${EXE_SUFFIX})

# Include modules from the SDK
include(ConvertElfToBin)
include(SignTfmImage)

# Test management
include(CTest)

if(${RTOS} STREQUAL "FREERTOS")
  # Patch the FreeRTOS target to match config requirements
  target_include_directories(freertos-kernel
      PUBLIC
          ${CMAKE_SOURCE_DIR}/bsp/freertos-config
  )
  target_link_libraries(freertos-kernel
      PUBLIC
          aws-configs
          iotsdk-cmsis-core-device
  )
endif()

# Patch the ethernet-lan91c111 target to enable multicast
if(TARGET ethernet-lan91c111)
    target_compile_definitions(ethernet-lan91c111
        INTERFACE
            LAN91C111_RFS_MULTICAST_SUPPORT
    )
endif()

# Declare RTOS interface target
add_library(cmsis-rtos-implementation INTERFACE)

target_link_libraries(cmsis-rtos-implementation
    INTERFACE
        cmsis-rtos-api
)

if(${RTOS} STREQUAL "FREERTOS")
    target_link_libraries(cmsis-rtos-api
        PUBLIC
            freertos-cmsis-rtos
            freertos-kernel-heap-3
    )
elseif(${RTOS} STREQUAL "RTX")
    target_link_libraries(cmsis-rtos-api
        PUBLIC
            cmsis-rtx
    )
    target_link_libraries(cmsis-rtos-implementation
        INTERFACE
            heap-alloc-wrapper
    )
elseif(${RTOS} STREQUAL "THREADX")
    target_link_libraries(cmsis-rtos-api
        PUBLIC
            threadx-cdi-port
    )
    target_link_libraries(cmsis-rtos-implementation
        INTERFACE
            heap-alloc-wrapper
    )
endif()

target_include_directories(iotsdk-cmsis-core-device INTERFACE bsp/cmsis-config)

target_include_directories(iotsdk-cmsis-core-device
    INTERFACE
        ${CMAKE_BINARY_DIR}/_deps/cmsis-5-src/CMSIS/Core/Include
        $<$<STREQUAL:${CMAKE_SYSTEM_PROCESSOR},cortex-m55>:${CMAKE_BINARY_DIR}/_deps/cmsis-5-src/Device/ARM/ARMCM55/Include>
        $<$<STREQUAL:${CMAKE_SYSTEM_PROCESSOR},cortex-m85>:${CMAKE_BINARY_DIR}/_deps/cmsis-5-src/Device/ARM/ARMCM85/Include>
        $<$<STREQUAL:${CMAKE_SYSTEM_PROCESSOR},cortex-m4>:${CMAKE_BINARY_DIR}/_deps/cmsis-5-src/Device/ARM/ARMCM4/Include>
)

target_compile_definitions(iotsdk-cmsis-core-device
    INTERFACE
        $<$<STREQUAL:${CMAKE_SYSTEM_PROCESSOR},cortex-m55>:CMSIS_device_header="${CMAKE_BINARY_DIR}/_deps/cmsis-5-src/Device/ARM/ARMCM55/Include/ARMCM55.h">
        $<$<STREQUAL:${CMAKE_SYSTEM_PROCESSOR},cortex-m85>:CMSIS_device_header="${CMAKE_BINARY_DIR}/_deps/cmsis-5-src/Device/ARM/ARMCM85/Include/ARMCM85.h">
        $<$<STREQUAL:${CMAKE_SYSTEM_PROCESSOR},cortex-m4>:CMSIS_device_header="${CMAKE_BINARY_DIR}/_deps/cmsis-5-src/Device/ARM/ARMCM4/Include/ARMCM4.h">
)

# Compilation Database
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

if(${CLOUD_CLIENT} STREQUAL "AWS")
  add_subdirectory(lib/AWS)
endif()
add_subdirectory(bsp)
add_subdirectory(lib/SpeexDSP)
add_subdirectory(lib/ml-kit)

# Setup Target

set(CMAKE_EXECUTABLE_SUFFIX ${EXE_SUFFIX})

# Cloud client setup
if(${CLOUD_CLIENT} STREQUAL "AWS")

    # Configure AWS libraries
    target_include_directories(coreHTTP-config
        INTERFACE
            ${CMAKE_SOURCE_DIR}/lib/AWS/aws_libraries/logging/include/
    )
    target_include_directories(coreMQTT-config
        INTERFACE
            ${CMAKE_SOURCE_DIR}/lib/AWS/aws_libraries/logging/include/
    )
    target_link_libraries(coreMQTT-config
        INTERFACE
            cmsis-rtos-api
            aws-configs
    )
    target_link_libraries(coreHTTP-config
        INTERFACE
            cmsis-rtos-api
            aws-configs
    )
elseif(${CLOUD_CLIENT} STREQUAL "AZURE")

    # Azure SDK config
    # Provides NTP implementation of time() function to Azure SDK platform library
    target_link_libraries(azure-iot-sdk-platform
        PUBLIC
            azure-sdk-ntp-time
    )

    # Provides LwIP socket implementation to Azure SDK platform library
    target_link_libraries(azure-iot-sdk-platform
        PUBLIC
            lwip-sockets
    )
endif()

# This function is used to generate the command line to convert a elf file into a plain binary
function(make_elf_to_bin_cmd cmd input output)
    if(CMAKE_C_COMPILER_ID STREQUAL "GNU")
        set(${cmd} ${GCC_ELF2BIN} -O binary ${input} ${output} PARENT_SCOPE)
    elseif(CMAKE_C_COMPILER_ID STREQUAL "ARMClang")
        set(${cmd} ${ARM_ELF2BIN} --bin --output ${output} ${input} --bincombined PARENT_SCOPE)
    endif()
endfunction()

# Add examples targets
add_subdirectory(examples/speech)
add_subdirectory(examples/keyword)
add_subdirectory(examples/blinky)

# Copy tf-m binaries at the root
FetchContent_GetProperties(trusted-firmware-m)

add_custom_target(tfm-binaries
  BYPRODUCTS
    ${CMAKE_BINARY_DIR}/bootloader/bl2.axf
    ${CMAKE_BINARY_DIR}/secure_partition/tfm_s.axf
    ${CMAKE_BINARY_DIR}/secure_partition/tfm_s_signed.bin

  COMMAND ${CMAKE_COMMAND} -E copy
    ${trusted-firmware-m_BINARY_DIR}/install/outputs/bl2.axf
    ${CMAKE_CURRENT_BINARY_DIR}/bootloader

  COMMAND ${CMAKE_COMMAND} -E copy
    ${trusted-firmware-m_BINARY_DIR}/install/outputs/tfm_s.axf
    ${trusted-firmware-m_BINARY_DIR}/install/outputs/tfm_s_signed.bin
    ${CMAKE_CURRENT_BINARY_DIR}/secure_partition
)

add_dependencies(tfm-binaries trusted-firmware-m-build)
add_dependencies(tfm-ns-interface tfm-binaries)
