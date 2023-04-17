# Copyright (c) 2021-2023 ARM Limited. All rights reserved.
# SPDX-License-Identifier: Apache-2.0

include(FetchContent)
FetchContent_Declare(
    coreMQTT-Agent
    GIT_REPOSITORY https://github.com/FreeRTOS/coreMQTT-Agent
    GIT_TAG        3b743173ddc7ec00d3073462847db15580a56617
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
        "${PRJ_DIR}/lib/AWS/aws_libraries/logging/include"
        "${aws-iot-device-sdk-embedded-c_SOURCE_DIR}/libraries/aws/ota-for-aws-iot-embedded-sdk/source/include"
        "${aws-iot-device-sdk-embedded-c_SOURCE_DIR}/libraries/aws/ota-for-aws-iot-embedded-sdk/source/portable/os"
        "${aws-iot-device-sdk-embedded-c_SOURCE_DIR}/libraries/standard/coreMQTT/source/include"
        "${aws-iot-device-sdk-embedded-c_SOURCE_DIR}/libraries/standard/coreMQTT/source/interface"
        "${coremqtt-agent_SOURCE_DIR}/source/include"
        "${TFM_PLATFORM_TARGET_DIR}/arm/mps3/an552/device/include"
)

target_link_libraries(coreMQTT-Agent
    PUBLIC
        cmsis-rtos-api
        coreJSON
        tinycbor
        aws-configs
)

target_compile_definitions(coreMQTT-Agent
    PUBLIC
        MQTT_AGENT_DO_NOT_USE_CUSTOM_CONFIG
)
