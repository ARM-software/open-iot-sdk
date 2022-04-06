# Copyright (c) 2021 ARM Limited. All rights reserved.
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.21)

include(ExternalProject)
include(FetchContent)

# Declare root and other build paths
set(PRJ_DIR "${CMAKE_CURRENT_SOURCE_DIR}")

# Declare relative path to TFM target
# Underscore are used to accomodate TF-M conversion in install folder from dash
# to underscore
set(TFM_PLATFORM_LOCAL_PATH "bsp/tf_m_targets/arm/mps3/an547")

# Declare the Open IoT SDK
FetchContent_Declare(
  open_iot_sdk
  GIT_REPOSITORY https://git.gitlab.arm.com/iot/open-iot-sdk/sdk.git
  GIT_TAG        v2022.01
)

# Declare the Open IoT SDK configuration flags
set(IOTSDK_FREERTOS ON)
set(IOTSDK_TFM ON)
set(IOTSDK_ML ON)
set(IOTSDK_CMSIS_RTOS_API ON)
set(IOTSDK_MDH_ARM ON)
set(FETCHCONTENT_QUIET OFF)
set(TFM_PLATFORM "${CMAKE_CURRENT_SOURCE_DIR}/${TFM_PLATFORM_LOCAL_PATH}")
set(TFM_PSA_API OFF)
set(TFM_PARTITION_FIRMWARE_UPDATE ON)

# Extra arguments for TF-M and ML
set(TFM_CMAKE_ARGS "-DNS=OFF;-DPLATFORM_DEFAULT_UART_STDOUT=ON;-DMCUBOOT_SIGNATURE_KEY_LEN=2048;-DTFM_SYSTEM_FP=2;-DTFM_SYSTEM_FP_NS=2;-DTFM_SYSTEM_FPU=2;-DTFM_PARTITION_AUDIT_LOG=ON;-DITS_MAX_ASSET_SIZE=1300;-DCRYPTO_ENGINE_BUF_SIZE=0x8000")
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

# Patch the FreeRTOS target to match config requirements
target_include_directories(freertos-kernel
    INTERFACE
        ${PRJ_DIR}/bsp/aws_configs
        ${PRJ_DIR}/bsp/freertos-platform
        ${PRJ_DIR}/kws/include
)

# Compilation Database
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
add_custom_target(database COMMAND ${CMAKE_COMMAND} -E copy_if_different "${INT_DIR}/compile_commands.json" "${CMAKE_BINARY_DIR}")

# handle libraries
set(MQTT_CONFIG_PATH ${PRJ_DIR}/bsp/aws_configs)
set(HTTP_CONFIG_PATH ${PRJ_DIR}/bsp/aws_configs)
set(PKCS11_CONFIG_PATH ${PRJ_DIR}/bsp/aws_configs)
set(MBEDTLS_CONFIG_PATH ${PRJ_DIR}/bsp/aws_libraries/3rdparty/mbedtls_config)

add_subdirectory(lib/lwip)
add_subdirectory(lib/AWS)
add_subdirectory(bsp)

# Setup Target

set(CMAKE_EXECUTABLE_SUFFIX ${EXE_SUFFIX})

# Add examples targets
add_subdirectory(kws)
add_subdirectory(blinky)

# Copy tf-m binaries at the root
ExternalProject_Get_Property(tf-m BINARY_DIR)
ExternalProject_Get_Property(tf-m SOURCE_DIR)
add_custom_target(tfm-binaries
  BYPRODUCTS
    ${CMAKE_BINARY_DIR}/bootloader/bl2.axf
    ${CMAKE_BINARY_DIR}/secure_partition/tfm_s.axf
    ${CMAKE_BINARY_DIR}/secure_partition/tfm_s_unsigned.bin
    ${CMAKE_BINARY_DIR}/secure_partition/tfm_s_signed.bin

  COMMAND ${CMAKE_COMMAND} -E copy
    ${BINARY_DIR}/install/outputs/$<UPPER_CASE:"EXTERNAL/${TFM_PLATFORM_LOCAL_PATH}">/bl2.axf
    "${CMAKE_CURRENT_BINARY_DIR}/bootloader/bl2.axf"

  COMMAND ${CMAKE_COMMAND} -E copy
    ${BINARY_DIR}/install/outputs/$<UPPER_CASE:"EXTERNAL/${TFM_PLATFORM_LOCAL_PATH}">/tfm_s.axf
    "${CMAKE_CURRENT_BINARY_DIR}/secure_partition/tfm_s.axf"

  COMMAND
    ${ARM_ELF2BIN} --bin --output ${CMAKE_BINARY_DIR}/secure_partition/tfm_s_unsigned.bin ${CMAKE_BINARY_DIR}/secure_partition/tfm_s.axf --bincombined

  COMMAND
    python3 ${BINARY_DIR}/install/image_signing/scripts/wrapper/wrapper.py
        -v 1.3.0
        --layout ${PRJ_DIR}/bsp/signing_layout_s.c
        -k ${SOURCE_DIR}/bl2/ext/mcuboot/root-RSA-2048.pem
        --public-key-format full
        --align 1
        --pad
        --pad-header
        -H 0x400
        -s auto
        ${CMAKE_BINARY_DIR}/secure_partition/tfm_s_unsigned.bin
        ${CMAKE_BINARY_DIR}/secure_partition/tfm_s_signed.bin
)

add_dependencies(tfm-binaries tf-m)
add_dependencies(tfm-ns-interface tfm-binaries)
