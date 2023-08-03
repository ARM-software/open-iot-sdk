# Copyright (c) 2022, Arm Limited and Contributors. All rights reserved.
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.18)

add_library(aws-configs INTERFACE)

target_include_directories(aws-configs
    INTERFACE
        .
)

target_compile_definitions(aws-configs
    INTERFACE
        LIBRARY_LOG_LEVEL=LOG_INFO
)
