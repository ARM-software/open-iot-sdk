# Copyright (c) 2021 ARM Limited. All rights reserved.
# SPDX-License-Identifier: Apache-2.0

add_library(backoff_algorithm STATIC EXCLUDE_FROM_ALL
    "${aws-iot-device-sdk_SOURCE_DIR}/libraries/standard/backoffAlgorithm/source/backoff_algorithm.c"
)

target_include_directories(backoff_algorithm
    PUBLIC
        ${aws-iot-device-sdk_SOURCE_DIR}/libraries/standard/backoffAlgorithm/source/include
)
