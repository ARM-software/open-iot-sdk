# Copyright (c) 2021 ARM Limited. All rights reserved.
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.18)

set(LIB_NAME coreMQTT)

include(${aws-iot-device-sdk_SOURCE_DIR}/libraries/standard/coreMQTT/mqttFilePaths.cmake)
include(${aws-iot-device-sdk_SOURCE_DIR}/demos/logging-stack/logging.cmake)

add_library(${LIB_NAME} STATIC EXCLUDE_FROM_ALL)

target_include_directories(${LIB_NAME}
    PUBLIC
        ${MQTT_INCLUDE_PUBLIC_DIRS}
        ${LOGGING_INCLUDE_DIRS}
        ${MQTT_CONFIG_PATH}
)

target_sources(${LIB_NAME}
    PRIVATE
        ${MQTT_SOURCES}
        ${MQTT_SERIALIZER_SOURCES}
)
