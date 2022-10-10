# Copyright (c) 2021 ARM Limited. All rights reserved.
# SPDX-License-Identifier: Apache-2.0

add_library(tinycbor STATIC EXCLUDE_FROM_ALL
    "${aws-iot-device-sdk_SOURCE_DIR}/libraries/3rdparty/tinycbor/src/cborencoder.c"
    "${aws-iot-device-sdk_SOURCE_DIR}/libraries/3rdparty/tinycbor/src/cborencoder_close_container_checked.c"
    "${aws-iot-device-sdk_SOURCE_DIR}/libraries/3rdparty/tinycbor/src/cborerrorstrings.c"
    "${aws-iot-device-sdk_SOURCE_DIR}/libraries/3rdparty/tinycbor/src/cborparser.c"
    "${aws-iot-device-sdk_SOURCE_DIR}/libraries/3rdparty/tinycbor/src/cborparser_dup_string.c"
    "${aws-iot-device-sdk_SOURCE_DIR}/libraries/3rdparty/tinycbor/src/cborpretty.c"
    "${aws-iot-device-sdk_SOURCE_DIR}/libraries/3rdparty/tinycbor/src/cborpretty_stdio.c"
    "${aws-iot-device-sdk_SOURCE_DIR}/libraries/3rdparty/tinycbor/src/cbortojson.c"
    "${aws-iot-device-sdk_SOURCE_DIR}/libraries/3rdparty/tinycbor/src/cborvalidation.c"
)

target_include_directories(tinycbor
    PUBLIC
        "${aws-iot-device-sdk_SOURCE_DIR}/libraries/3rdparty/tinycbor/src"
)
