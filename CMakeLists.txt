# Copyright (c) 2021-2022, Arm Limited and Contributors. All rights reserved.
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.21)

include(ExternalProject)
include(FetchContent)

# CMake processes toolchain file during first call to project(), so it needs to exist first
FetchContent_Declare(
    iotsdk-toolchains
    GIT_REPOSITORY  https://git.gitlab.arm.com/iot/open-iot-sdk/toolchain.git
    GIT_TAG         c82c133e270e5631e3ba355fa0baa626123def0f
    SOURCE_DIR      ${CMAKE_BINARY_DIR}/toolchains
)
FetchContent_MakeAvailable(iotsdk-toolchains)

# Start of the project
project(keyword LANGUAGES C CXX)

# Declare root and other build paths
set(PRJ_DIR "${CMAKE_CURRENT_SOURCE_DIR}")

# Declare the target of the total solution
set(TS_TARGET "Corstone-300" CACHE STRING "Hardware target of the Total Solution")

# Configure target
# - Declare relative path to TFM target
# - MDH platform
if (${TS_TARGET} STREQUAL "Corstone-300")
    set(TFM_PLATFORM_LOCAL_PATH "bsp/tf_m_targets/arm/mps3/an547")
    set(MDH_PLATFORM "ARM_AN552_MPS3")
    set(ML_TARGET_SUBSYSTEM "sse-300")
    set(ETHOS_U_NPU_CONFIG_ID "H128")
    set(ETHOSU_TARGET_NPU_CONFIG "ethos-u55-128")
elseif(${TS_TARGET} STREQUAL "Corstone-310")
    set(TFM_PLATFORM_LOCAL_PATH "bsp/tf_m_targets/arm/mps3/corstone_polaris")
    set(MDH_PLATFORM "ARM_AN555_MPS3")
    set(ML_TARGET_SUBSYSTEM "sse-310")
    set(ETHOS_U_NPU_CONFIG_ID "H256")
    set(ETHOSU_TARGET_NPU_CONFIG "ethos-u55-256")
else()
    message(FATAL_ERROR "Invalid TS_TARGET (${TS_TARGET}) set. It should be Corstone-300 or Corstone-310")
endif()

# Set build type to prevent MCU Driver HAL to set it to Develop by default which is not correct
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "Debug" CACHE STRING "The build type" FORCE)
endif()

set(FETCHCONTENT_QUIET OFF)

# Declare the Open IoT SDK
FetchContent_Declare(
  open_iot_sdk
  GIT_REPOSITORY https://git.gitlab.arm.com/iot/open-iot-sdk/sdk.git
  GIT_TAG        546e9fef3e7842ebb200efa079a7753c2e4b9ee9
)

# Select Open IoT SDK components

set(IOTSDK_FETCH_LIST
    trusted-firmware-m
    ml-embedded-evaluation-kit
    cmsis-5
    mcu-driver-reference-platforms-for-arm
    mbedtls
    lwip
    cmsis-sockets-api
)

if(${RTOS} STREQUAL "FREERTOS")
    list(APPEND IOTSDK_FETCH_LIST cmsis-freertos)
elseif(${RTOS} STREQUAL "RTX")
    list(APPEND IOTSDK_FETCH_LIST cmsis-5)
elseif(${RTOS} STREQUAL "THREADX")
    list(APPEND IOTSDK_FETCH_LIST threadx threadx-cdi-port)
endif()

if(${CLOUD_CLIENT} STREQUAL "AWS")
    list(APPEND IOTSDK_FETCH_LIST aws-iot-device-sdk-embedded-c)
elseif(${CLOUD_CLIENT} STREQUAL "AZURE")
    list(APPEND IOTSDK_FETCH_LIST azure-iot-sdk-c)
elseif(${CLOUD_CLIENT} STREQUAL "AZURE_NETXDUO")
    list(APPEND IOTSDK_FETCH_LIST netxduo)
    # By default, NetX Duo changes WARNINGS_AS_ERRORS to ON
    set(WARNINGS_AS_ERRORS OFF)
endif()


# Configure TrustedFirmware-M
set(TFM_PLATFORM "${CMAKE_CURRENT_SOURCE_DIR}/${TFM_PLATFORM_LOCAL_PATH}")
set(TFM_PARTITION_FIRMWARE_UPDATE ON)
set(CONFIG_TFM_ENABLE_FPU ON)
set(TFM_CMAKE_ARGS "-DNS=ON;-DPLATFORM_DEFAULT_UART_STDOUT=ON;-DMCUBOOT_SIGNATURE_KEY_LEN=2048;-DITS_MAX_ASSET_SIZE=1300;-DCRYPTO_ENGINE_BUF_SIZE=0x8000;-DMCUBOOT_LOG_LEVEL=INFO;-DTFM_SPM_LOG_LEVEL=TFM_SPM_LOG_LEVEL_INFO")

# Extra arguments for ML
set(ML_CMAKE_ARGS "-DTARGET_SUBSYSTEM=${ML_TARGET_SUBSYSTEM};-DETHOS_U_NPU_CONFIG_ID=${ETHOS_U_NPU_CONFIG_ID};-DETHOSU_TARGET_NPU_CONFIG=${ETHOSU_TARGET_NPU_CONFIG}")
set(ML_TARGETS cmsis-dsp tensorflow_build kws asr)
if(${TS_TARGET} STREQUAL "Corstone-300")
    list(APPEND ML_TARGETS timing_adapter)
endif()

# Apply ML patch to use tiny asr model
find_package(Git)
set(ML_PATCH_COMMAND ${GIT_EXECUTABLE} apply "${PRJ_DIR}/lib/ml-kit/ml-embedded-evaluation-kit.patch" || true)

# Set library to std
set(MBED_C_LIB "std")

# Set FVP variant
set(VARIANT "FVP")

# ThreadX (if enabled) will use TX_USER_FILE to locate its configuration file
# Note that this must be set before the IoT SDK is added
set(TX_USER_FILE "${PRJ_DIR}/bsp/threadx-config/tx_user.h")

set(NXD_ENABLE_AZURE_IOT ON CACHE BOOL "")
set(NXD_ENABLE_FILE_SERVERS OFF CACHE BOOL "")
# NetX Duo (if enabled) will use NX_USER_FILE to locate its configuration file
# Note that this must be set before the IoT SDK is added
set(NX_USER_FILE "${PRJ_DIR}/bsp/netxduo-config/nx_user.h")

# Make the Open IoT SDK available
FetchContent_MakeAvailable(open_iot_sdk)

# Fetch project_options
FetchContent_Declare(
    project_options
    GIT_REPOSITORY  https://github.com/cpp-best-practices/project_options.git
    GIT_TAG         v0.21.0
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
set(APP_CONFIG_CREDENTIALS_PATH "${PRJ_DIR}/bsp/default_credentials" CACHE STRING "Path to credentials")

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
          ${PRJ_DIR}/bsp/freertos-config
  )
  target_link_libraries(freertos-kernel
      PUBLIC
          aws-configs
          cmsis-core
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


# CMSIS 5 require projects to provide configuration macros via RTE_Components.h
# and CMSIS_device_header. The macro CMSIS_device_header is not automatically set
# based on CMAKE_SYSTEM_PROCESSOR in the place where cmsis-core is first defined,
# because a project may want to provide its own device header.
target_include_directories(cmsis-core
    INTERFACE
        bsp/cmsis-config
)
target_compile_definitions(cmsis-core
    INTERFACE
        $<$<STREQUAL:${CMAKE_SYSTEM_PROCESSOR},cortex-m55>:CMSIS_device_header="ARMCM55.h">
        $<$<STREQUAL:${CMAKE_SYSTEM_PROCESSOR},cortex-m85>:CMSIS_device_header="ARMCM85.h">
)

# It is required to pass to mcu-driver-hal that it is compiled in NS mode
target_compile_definitions(mcu-driver-hal
    INTERFACE
        DOMAIN_NS=1
)

# Compilation Database
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
add_custom_target(database COMMAND ${CMAKE_COMMAND} -E copy_if_different "${INT_DIR}/compile_commands.json" "${CMAKE_BINARY_DIR}")

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
            ${PRJ_DIR}/lib/AWS/aws_libraries/logging/include/
    )
    target_include_directories(coreMQTT-config
        INTERFACE
            ${PRJ_DIR}/lib/AWS/aws_libraries/logging/include/
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
    ${CMAKE_BINARY_DIR}/secure_partition/tfm_s_unsigned.bin
    ${CMAKE_BINARY_DIR}/secure_partition/tfm_s_signed.bin

  COMMAND ${CMAKE_COMMAND} -E copy
    ${trusted-firmware-m_BINARY_DIR}/install/outputs/bl2.axf
    "${CMAKE_CURRENT_BINARY_DIR}/bootloader/bl2${CMAKE_EXECUTABLE_SUFFIX}"

  COMMAND ${CMAKE_COMMAND} -E copy
    ${trusted-firmware-m_BINARY_DIR}/install/outputs/tfm_s.axf
    "${CMAKE_CURRENT_BINARY_DIR}/secure_partition/tfm_s${CMAKE_EXECUTABLE_SUFFIX}"

  COMMAND
    ${ARM_ELF2BIN} --bin --output ${CMAKE_BINARY_DIR}/secure_partition/tfm_s_unsigned.bin ${CMAKE_BINARY_DIR}/secure_partition/tfm_s${CMAKE_EXECUTABLE_SUFFIX} --bincombined

  COMMAND
    python3 ${trusted-firmware-m_BINARY_DIR}/install/image_signing/scripts/wrapper/wrapper.py
        -v 0.0.1
        --layout ${PRJ_DIR}/bsp/signing_layout_s.c
        -k ${trusted-firmware-m_SOURCE_DIR}/bl2/ext/mcuboot/root-RSA-2048.pem
        --public-key-format full
        --align 1
        --pad
        --pad-header
        -H 0x400
        -s auto
        ${CMAKE_BINARY_DIR}/secure_partition/tfm_s_unsigned.bin
        ${CMAKE_BINARY_DIR}/secure_partition/tfm_s_signed.bin
)

add_dependencies(tfm-binaries trusted-firmware-m-build)
add_dependencies(tfm-ns-interface tfm-binaries)
