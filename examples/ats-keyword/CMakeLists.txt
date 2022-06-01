# Copyright (c) 2021-2022, Arm Limited and Contributors. All rights reserved.
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.21)

include(ExternalProject)
include(FetchContent)

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
elseif(${TS_TARGET} STREQUAL "Corstone-310")
    set(TFM_PLATFORM_LOCAL_PATH "bsp/tf_m_targets/arm/mps3/corstone_polaris")
    set(MDH_PLATFORM "ARM_AN555_MPS3")
else()
    message(FATAL_ERROR "Invalid TS_TARGET (${TS_TARGET}) set. It should be Corstone-300 or Corstone-310")
endif()

# Set build type to prevent MCU Driver HAL to set it to Develop by default which is not correct
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "Debug" CACHE STRING "The build type" FORCE)
endif()


# Override TF-M version
FetchContent_Declare(
    tf-m
    GIT_REPOSITORY  https://git.trustedfirmware.org/TF-M/trusted-firmware-m.git
    # Note: AN552 currently requires an unreleased version of TF-M.
    # To be able to checkout an untagged commit, a full git clone is
    # needed and GIT_SHALLOW is disabled.
    GIT_TAG         TF-Mv1.6.0
    GIT_PROGRESS    ON
    # Note: This prevents FetchContent_MakeAvailable() from calling
    # add_subdirectory() on the fetched repository. TF-M needs a
    # standalone build because it relies on functions defined in its
    # own toolchain files and contains paths that reference the
    # top-level project instead of its own project.
    SOURCE_SUBDIR   NONE
)

FetchContent_Declare(iotsdk-toolchains
    GIT_REPOSITORY  https://git.gitlab.arm.com/iot/open-iot-sdk/toolchain.git
    GIT_TAG         126d2092a2325c77fdf36043e931293c62e2ad29
    SOURCE_DIR      ${CMAKE_BINARY_DIR}/toolchains
)

# Override MCU driver HAL
FetchContent_Declare(
  mcu-driver-reference-platforms-for-arm
  GIT_REPOSITORY https://git.gitlab.arm.com/iot/open-iot-sdk/mcu-driver-hal/mcu-driver-reference-platforms-for-arm.git
  GIT_TAG        9b7696596ca1c0f380e77f3d7c5de150635e340e
  GIT_PROGRESS   ON
)


# Declare the Open IoT SDK
FetchContent_Declare(
  open_iot_sdk
  GIT_REPOSITORY https://git.gitlab.arm.com/iot/open-iot-sdk/sdk.git
  GIT_TAG        dc62ef0bb0bc39f1ed67c33758e36482d3c6b2ff
)

# Declare the Open IoT SDK configuration flags
if(${RTOS} STREQUAL "FREERTOS")
  set(IOTSDK_FREERTOS ON)
elseif(${RTOS} STREQUAL "RTX")
  set(IOTSDK_CMSIS_RTX ON)
endif()

set(IOTSDK_TFM ON)
set(IOTSDK_ML ON)
set(IOTSDK_CMSIS_RTOS_API ON)
set(IOTSDK_MDH_ARM ON)
set(FETCHCONTENT_QUIET OFF)
set(TFM_PLATFORM "${CMAKE_CURRENT_SOURCE_DIR}/${TFM_PLATFORM_LOCAL_PATH}")
set(TFM_PSA_API OFF)
set(TFM_PARTITION_FIRMWARE_UPDATE ON)
set(TFM_LIB_MODEL ON)
set(IOTSDK_MBEDTLS ON)
set(IOTSDK_AWS_CLIENT ON)
set(IOTSDK_LWIP ON)
set(IOTSDK_LWIP_SOCKETS ON)
set(IOTSDK_IP_NETWORK ON)
set(IOTSDK_LWIP_CMSIS_IP_NETWORK ON)
set(IOTSDK_EXAMPLES OFF)

# Extra arguments for TF-M and ML
set(TFM_CMAKE_ARGS "-DNS=ON;-DPLATFORM_DEFAULT_UART_STDOUT=ON;-DMCUBOOT_SIGNATURE_KEY_LEN=2048;-DCONFIG_TFM_FP=hard;-DCONFIG_TFM_FP_ARCH=fpv5-sp-d16;TFM_SYSTEM_FP=ON;-DTFM_PARTITION_AUDIT_LOG=ON;-DITS_MAX_ASSET_SIZE=1300;-DCRYPTO_ENGINE_BUF_SIZE=0x8000;-DMCUBOOT_LOG_LEVEL=INFO;-DTFM_SPM_LOG_LEVEL=TFM_SPM_LOG_LEVEL_INFO")
set(ML_CMAKE_ARGS "-Dkws_LABELS_TXT_FILE=resources/kws/labels/ds_cnn_labels.txt;-Dkws_MODEL_TFLITE_PATH=resources_downloaded/kws/ds_cnn_clustered_int8_vela_H128.tflite;-DUSE_CASE_BUILD=kws;-Dkws_ACTIVATION_BUF_SZ=0x00400000")

# Set library to std
set(MBED_C_LIB "std")

# Set FVP variant
set(VARIANT "FVP")

# Apply TF-M patch
find_package(Git)
set(TFM_PATCH_COMMAND ${GIT_EXECUTABLE} apply "${PRJ_DIR}/patches/tf-m.patch" || true)
set(ML_PATCH_COMMAND ${GIT_EXECUTABLE} apply "${PRJ_DIR}/patches/ml-embedded-evaluation-kit.patch" || true)

# Make the Open IoT SDK available
FetchContent_MakeAvailable(open_iot_sdk)

# get paths for the ml kit

ExternalProject_Get_Property(ml-embedded-evaluation-kit SOURCE_DIR)
ExternalProject_Get_Property(ml-embedded-evaluation-kit BINARY_DIR)
set(ml-embedded-evaluation-kit_SOURCE_DIR  "${SOURCE_DIR}")
set(ml-embedded-evaluation-kit_BINARY_DIR  "${BINARY_DIR}")
set(ml-embedded-evaluation-kit_GENERATED_DIR "${ml-embedded-evaluation-kit_BINARY_DIR}/generated")
set(ml-embedded-evaluation-kit_LIB_DIR ${ml-embedded-evaluation-kit_BINARY_DIR}/lib)

# Setup project

# Start of the project
project(tfm_ns LANGUAGES C CXX)

# Path to AWS credentials
set(AWS_CONFIG_CREDENTIALS_PATH "${PRJ_DIR}/bsp/default_credentials" CACHE STRING "Path to AWS credentials")

# Include module path from Open IoT SDK
list(APPEND CMAKE_MODULE_PATH ${open_iot_sdk_SOURCE_DIR}/components/TF-M)
list(APPEND CMAKE_MODULE_PATH ${open_iot_sdk_SOURCE_DIR}/cmake)

# Configuration of executable
set(EXE_SUFFIX ".axf")
set(CMAKE_EXECUTABLE_SUFFIX ${EXE_SUFFIX})

# Include modules from the SDK
include(ElfToBin)
include(TfmSignImage)

# Test management
include(CTest)

if(TARGET freertos-kernel)
  # Patch the FreeRTOS target to match config requirements
  target_include_directories(freertos-kernel
      INTERFACE
          ${PRJ_DIR}/bsp/aws_configs
          ${PRJ_DIR}/bsp/freertos-config
          ${PRJ_DIR}/bsp/freertos-platform
          ${PRJ_DIR}/kws/include
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

if(TARGET freertos-kernel)
    target_link_libraries(cmsis-rtos-implementation
        INTERFACE
            freertos-cmsis-rtos
            freertos-kernel-heap-3
    )
elseif(TARGET cmsis-rtx)
    target_link_libraries(cmsis-rtos-implementation
        INTERFACE
            cmsis-rtx
            cmsis-rtos-api
            cmsis-rtx-freertos-alloc-wrapper
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

# handle libraries
set(MQTT_CONFIG_PATH ${PRJ_DIR}/bsp/aws_configs)
set(HTTP_CONFIG_PATH ${PRJ_DIR}/bsp/aws_configs)
set(PKCS11_CONFIG_PATH ${PRJ_DIR}/bsp/aws_configs)
set(MBEDTLS_CONFIG_PATH ${PRJ_DIR}/bsp/aws_libraries/3rdparty/mbedtls_config)

add_subdirectory(lib/AWS)
add_subdirectory(bsp)

# Setup Target

set(CMAKE_EXECUTABLE_SUFFIX ${EXE_SUFFIX})

# Add examples targets
add_subdirectory(kws)
add_subdirectory(blinky)

# Copy tf-m binaries at the root
FetchContent_GetProperties(tf-m)

add_custom_target(tfm-binaries
  BYPRODUCTS
    ${CMAKE_BINARY_DIR}/bootloader/bl2.axf
    ${CMAKE_BINARY_DIR}/secure_partition/tfm_s.axf
    ${CMAKE_BINARY_DIR}/secure_partition/tfm_s_unsigned.bin
    ${CMAKE_BINARY_DIR}/secure_partition/tfm_s_signed.bin

  COMMAND ${CMAKE_COMMAND} -E copy
    ${tf-m_BINARY_DIR}/install/outputs/bl2.axf
    "${CMAKE_CURRENT_BINARY_DIR}/bootloader/bl2${CMAKE_EXECUTABLE_SUFFIX}"

  COMMAND ${CMAKE_COMMAND} -E copy
    ${tf-m_BINARY_DIR}/install/outputs/tfm_s.axf
    "${CMAKE_CURRENT_BINARY_DIR}/secure_partition/tfm_s${CMAKE_EXECUTABLE_SUFFIX}"

  COMMAND
    ${ARM_ELF2BIN} --bin --output ${CMAKE_BINARY_DIR}/secure_partition/tfm_s_unsigned.bin ${CMAKE_BINARY_DIR}/secure_partition/tfm_s${CMAKE_EXECUTABLE_SUFFIX} --bincombined

  COMMAND
    python3 ${tf-m_BINARY_DIR}/install/image_signing/scripts/wrapper/wrapper.py
        -v 0.0.1
        --layout ${PRJ_DIR}/bsp/signing_layout_s.c
        -k ${tf-m_SOURCE_DIR}/bl2/ext/mcuboot/root-RSA-2048.pem
        --public-key-format full
        --align 1
        --pad
        --pad-header
        -H 0x400
        -s auto
        ${CMAKE_BINARY_DIR}/secure_partition/tfm_s_unsigned.bin
        ${CMAKE_BINARY_DIR}/secure_partition/tfm_s_signed.bin
)

add_dependencies(tfm-binaries tf-m-build)
add_dependencies(tfm-ns-interface tfm-binaries)
