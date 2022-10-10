# Copyright (c) 2021 ARM Limited. All rights reserved.
# SPDX-License-Identifier: Apache-2.0

add_library(PKCS11 STATIC EXCLUDE_FROM_ALL
    # PKCS 11
    "${aws-iot-device-sdk_SOURCE_DIR}/libraries/standard/corePKCS11/source/core_pkcs11.c"
    "${aws-iot-device-sdk_SOURCE_DIR}/libraries/standard/corePKCS11/source/core_pki_utils.c"

    # PKCS11 - PSA
    "../aws_libraries/abstractions/pkcs11/psa/iot_pkcs11_psa.c"
    "../aws_libraries/abstractions/pkcs11/psa/iot_pkcs11_psa_input_format.c"
    "../aws_libraries/demos/common/pkcs11_helpers/pkcs11_helpers.c"
    "../aws_libraries/abstractions/pkcs11/psa/iot_pkcs11_psa_object_management.c"
)

target_include_directories(PKCS11
    PUBLIC
        "../aws_libraries/demos/common/pkcs11_helpers"
        "../aws_libraries/abstractions/platform/freertos/include"
        "../aws_libraries/abstractions/platform/freertos/include/platform"
        "../aws_libraries/c_sdk/standard/common/include/private"
        "${aws-iot-device-sdk_SOURCE_DIR}/libraries/standard/corePKCS11/source/include"
        "${aws-iot-device-sdk_SOURCE_DIR}/libraries/standard/corePKCS11/source/dependency/3rdparty/pkcs11"
        "../aws_libraries/abstractions/pkcs11/corePKCS11/source/dependency/3rdparty/mbedtls_utils"
    PRIVATE
        "../aws_libraries/3rdparty/pkcs11"
        "../aws_libraries/logging/include"
        "../aws_libraries/abstractions/pkcs11/corePKCS11/source/dependency/3rdparty/pkcs11"
        "../aws_libraries/abstractions/pkcs11/psa"
        "../aws_libraries/c_sdk/standard/https/src/pkcs11/corePKCS11/source/include"
        "../aws_libraries/freertos_plus/standard/crypto/include"
        "../aws_libraries/abstractions/pkcs11/corePKCS11/source/include"
)

target_link_libraries(PKCS11
    PUBLIC
        cmsis-rtos-api
        tfm-ns-interface
        mbedtls
        aws-configs
        mbedtls-config
        cloud-credentials
)
