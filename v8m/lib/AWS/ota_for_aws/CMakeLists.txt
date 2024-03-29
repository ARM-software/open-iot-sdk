# Copyright (c) 2021-2023 ARM Limited. All rights reserved.
# SPDX-License-Identifier: Apache-2.0

add_library(ota_for_aws STATIC EXCLUDE_FROM_ALL
    "${aws-iot-device-sdk-embedded-c_SOURCE_DIR}/libraries/aws/ota-for-aws-iot-embedded-sdk/source/ota.c"
    "${aws-iot-device-sdk-embedded-c_SOURCE_DIR}/libraries/aws/ota-for-aws-iot-embedded-sdk/source/ota_base64.c"
    "${aws-iot-device-sdk-embedded-c_SOURCE_DIR}/libraries/aws/ota-for-aws-iot-embedded-sdk/source/ota_cbor.c"
    "${aws-iot-device-sdk-embedded-c_SOURCE_DIR}/libraries/aws/ota-for-aws-iot-embedded-sdk/source/ota_http.c"
    "${aws-iot-device-sdk-embedded-c_SOURCE_DIR}/libraries/aws/ota-for-aws-iot-embedded-sdk/source/ota_interface.c"
    "${aws-iot-device-sdk-embedded-c_SOURCE_DIR}/libraries/aws/ota-for-aws-iot-embedded-sdk/source/ota_mqtt.c"
    "ota_os_cmsisrtos.c"
)

target_include_directories(ota_for_aws
    PUBLIC
        "${PRJ_DIR}/lib/AWS/aws_libraries/freertos_plus/aws/ota/include"
        "${PRJ_DIR}/lib/AWS/aws_libraries/freertos_plus/aws/ota/src"
        "${PRJ_DIR}/lib/AWS/aws_libraries/freertos_plus/aws/ota/src/http"
        "${PRJ_DIR}/lib/AWS/aws_libraries/freertos_plus/aws/ota/src/mqtt"
        "${PRJ_DIR}/lib/AWS/aws_libraries/logging/include"
        "${PRJ_DIR}/lib/AWS/ota/ota_pal_psa"
        "${PRJ_DIR}/lib/AWS/ota/ota_pal_psa/version"
        "${PRJ_DIR}/lib/AWS/ota/provision"
        "${aws-iot-device-sdk-embedded-c_SOURCE_DIR}/libraries/aws/ota-for-aws-iot-embedded-sdk/source/include"
        "${aws-iot-device-sdk-embedded-c_SOURCE_DIR}/libraries/aws/ota-for-aws-iot-embedded-sdk/source/portable/os"
        "${TFM_PLATFORM_TARGET_DIR}/arm/mps3/an552/device/include"
)

target_link_libraries(ota_for_aws
    PUBLIC
        cmsis-rtos-api
        tinycbor
        coreJSON
        aws-configs
)
