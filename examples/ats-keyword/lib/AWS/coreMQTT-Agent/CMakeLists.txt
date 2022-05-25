# Copyright (c) 2021 ARM Limited. All rights reserved.
# SPDX-License-Identifier: Apache-2.0

include(FetchContent)
FetchContent_Declare(
    coreMQTT-Agent
    GIT_REPOSITORY https://github.com/FreeRTOS/coreMQTT-Agent
    GIT_TAG        505dd2343af5ffd84f865ae83938edc45b4f2baa
    GIT_PROGRESS   ON
)
FetchContent_Populate(coreMQTT-Agent)

set(coremqtt-agent_SOURCE_DIR ${coremqtt-agent_SOURCE_DIR} PARENT_SCOPE)

add_library(coreMQTT-Agent STATIC EXCLUDE_FROM_ALL
    "${coremqtt-agent_SOURCE_DIR}/source/core_mqtt_agent.c"
    "${coremqtt-agent_SOURCE_DIR}/source/core_mqtt_agent_command_functions.c"
)

target_include_directories(coreMQTT-Agent
    PUBLIC
        "${PRJ_DIR}/bsp/aws_configs"
        "${PRJ_DIR}/bsp/aws_libraries/logging/include"
        "${aws-iot-device-sdk_SOURCE_DIR}/libraries/aws/ota-for-aws-iot-embedded-sdk/source/include"
        "${aws-iot-device-sdk_SOURCE_DIR}/libraries/aws/ota-for-aws-iot-embedded-sdk/source/portable/os"
        "${aws-iot-device-sdk_SOURCE_DIR}/libraries/standard/coreMQTT/source/include"
        "${aws-iot-device-sdk_SOURCE_DIR}/libraries/standard/coreMQTT/source/interface"
        "${coremqtt-agent_SOURCE_DIR}/source/include"
        "${TFM_PLATFORM_TARGET_DIR}/arm/mps3/an547/device/include"
)

target_link_libraries(coreMQTT-Agent
    PUBLIC
        cmsis-rtos-api
        coreJSON
        tinycbor
)
