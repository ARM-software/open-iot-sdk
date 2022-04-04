# Copyright (c) 2021 ARM Limited. All rights reserved.
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.18)

set(LIB_NAME coreHTTP)

include(${aws-iot-device-sdk_SOURCE_DIR}/libraries/standard/coreHTTP/httpFilePaths.cmake)
include(${aws-iot-device-sdk_SOURCE_DIR}/demos/logging-stack/logging.cmake)

add_library(${LIB_NAME} STATIC EXCLUDE_FROM_ALL)

target_include_directories(${LIB_NAME}
    PUBLIC
        ${HTTP_INCLUDE_PUBLIC_DIRS}
        ${LOGGING_INCLUDE_DIRS}
        ${HTTP_CONFIG_PATH}
)

target_sources(${LIB_NAME}
    PRIVATE
        ${HTTP_SOURCES}
)
