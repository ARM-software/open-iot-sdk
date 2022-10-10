# Copyright (c) 2022, Arm Limited and Contributors. All rights reserved.
# SPDX-License-Identifier: Apache-2.0

target_link_libraries(mbedtls-config
    INTERFACE
        mbedtls-threading-cmsis-rtos
)

target_include_directories(mbedtls-config
    INTERFACE
        .
)

target_compile_definitions(mbedtls-config
    INTERFACE
        MBEDTLS_CONFIG_FILE="aws_mbedtls_config.h"
)
