# Copyright (c) 2021 ARM Limited. All rights reserved.
# SPDX-License-Identifier: Apache-2.0

add_library(PKCS11 STATIC EXCLUDE_FROM_ALL
    # PKCS 11
    "${aws-iot-device-sdk_SOURCE_DIR}/libraries/standard/corePKCS11/source/core_pkcs11.c"
    "${aws-iot-device-sdk_SOURCE_DIR}/libraries/standard/corePKCS11/source/core_pki_utils.c"

    # PKCS11 - PSA
    "${PRJ_DIR}/bsp/aws_libraries/abstractions/pkcs11/psa/iot_pkcs11_psa.c"
    "${PRJ_DIR}/bsp/aws_libraries/abstractions/pkcs11/psa/iot_pkcs11_psa_input_format.c"
    "${PRJ_DIR}/bsp/aws_libraries/demos/common/pkcs11_helpers/pkcs11_helpers.c"
    "${PRJ_DIR}/bsp/aws_libraries/abstractions/pkcs11/psa/iot_pkcs11_psa_object_management.c"
)

target_include_directories(PKCS11
    PUBLIC
        "${PRJ_DIR}/bsp/aws_libraries/demos/common/pkcs11_helpers"
        "${PRJ_DIR}/bsp/aws_libraries/abstractions/platform/freertos/include"
        "${PRJ_DIR}/bsp/aws_libraries/abstractions/platform/freertos/include/platform"
        "${PRJ_DIR}/bsp/aws_libraries/c_sdk/standard/common/include/private"
        "${aws-iot-device-sdk_SOURCE_DIR}/libraries/standard/corePKCS11/source/include"
        "${aws-iot-device-sdk_SOURCE_DIR}/libraries/standard/corePKCS11/source/dependency/3rdparty/pkcs11"
        "${PRJ_DIR}/bsp/aws_libraries/3rdparty/mbedtls_config"
        "${PRJ_DIR}/bsp/aws_libraries/abstractions/pkcs11/corePKCS11/source/dependency/3rdparty/mbedtls_utils"
        ${AWS_CONFIG_CREDENTIALS_PATH}
    PRIVATE
        "${PRJ_DIR}/bsp/aws_libraries/3rdparty/pkcs11"
        "${PRJ_DIR}/bsp/aws_libraries/logging/include"
        "${PRJ_DIR}/bsp/aws_libraries/abstractions/pkcs11/corePKCS11/source/dependency/3rdparty/pkcs11"
        "${PRJ_DIR}/bsp/aws_libraries/abstractions/pkcs11/psa"
        "${PRJ_DIR}/bsp/aws_libraries/c_sdk/standard/https/src/pkcs11/corePKCS11/source/include"
        "${PRJ_DIR}/bsp/aws_libraries/freertos_plus/standard/crypto/include"
        "${PRJ_DIR}/bsp/aws_libraries/abstractions/pkcs11/corePKCS11/source/include"
        "${PRJ_DIR}/bsp/aws_configs"
)

target_link_libraries(PKCS11
    PUBLIC
        cmsis-rtos-api
        tfm-ns-interface
        mbedtls
)
