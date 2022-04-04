# Copyright (c) 2021 ARM Limited. All rights reserved.
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.18)

set(LIB_NAME coreJSON)

include(${aws-iot-device-sdk_SOURCE_DIR}/libraries/standard/coreJSON/jsonFilePaths.cmake)

add_library(${LIB_NAME} STATIC EXCLUDE_FROM_ALL)

target_include_directories(${LIB_NAME}
    PUBLIC
        ${JSON_INCLUDE_PUBLIC_DIRS}
)

target_sources(${LIB_NAME}
    PRIVATE
        ${JSON_SOURCES}
)
