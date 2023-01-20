# Copyright (c) 2021-2022 ARM Limited. All rights reserved.
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.21)

FetchContent_GetProperties(lwip-cmsis-port)

# LWIP
# lwipcore requires the config defined by lwip-cmsis-port
target_link_libraries(lwipcore
    PRIVATE
        lwip-cmsis-port
        iotsdk-emac-factory
)

target_include_directories(lwipopts
    INTERFACE
        ${PRJ_DIR}/bsp/lwip-config
)

# provide method to use for tracing by the lwip port (optional)
target_compile_definitions(lwipopts
    INTERFACE
        DEBUG_PRINT=printf
)

# Bind socket interface to implementation
target_link_libraries(cmsis-sockets-api INTERFACE lwip-sockets)

add_executable(keyword
    # application
    source/ml_interface.cc
    source/model_config.cc
    source/blink_task.c
    source/main_ns.c
)

target_compile_options(keyword
    PRIVATE
        $<$<COMPILE_LANGUAGE:CXX>:-std=gnu++14>
        $<$<COMPILE_LANGUAGE:C>:-std=gnu11>
)

target_include_directories(keyword
    PRIVATE
        source
        include
)

# final binary
target_link_libraries(keyword
    startup
    tfm-ns-interface-cmsis-rtos
    cmsis-rtos-implementation
    mcu-driver-hal
    ts-bsp
    ml-kit-kws

    project_options
    project_warnings
)

if(${CLOUD_CLIENT} STREQUAL "AWS")
    # Complete keyword
    target_sources(keyword
        PRIVATE
            source/aws_demo.c
            source/ethosu_platform_adaptation.c
    )

    target_link_libraries(keyword
        AWS-extra
        aws-configs
        mbedtls-config
    )
elseif(${CLOUD_CLIENT} STREQUAL "AZURE")
    target_sources(keyword
        PRIVATE
            source/azure_demo.c
    )

    # final binary
    target_link_libraries(keyword
        mbedtls
        tfm-ns-interface
        lwip-sockets
        lwipcore
        lwipopts
        iotsdk-ip-network-api
        cmsis-sockets-api
        mcu-driver-bootstrap
        mcu-driver-hal
        cmsis-rtos-api
        azure-iothub-client
        azure-provisioning-client
        cloud-credentials
    )
elseif(${CLOUD_CLIENT} STREQUAL "AZURE_NETXDUO")
    target_sources(keyword
        PRIVATE
            source/azure_netxduo_demo.c
            ${PRJ_DIR}/bsp/netxduo-config/nx_azure_iot_cert.c
            ${PRJ_DIR}/bsp/netxduo-config/nx_azure_iot_ciphersuites.c
            ${PRJ_DIR}/bsp/netxduo-config/sample_azure_iot_embedded_sdk_connect.c
    )

    target_include_directories(keyword
        PRIVATE
            ${APP_CONFIG_CREDENTIALS_PATH}
    )

    target_include_directories(keyword
        PRIVATE
            ${PRJ_DIR}/bsp/netxduo-config
            ${PRJ_DIR}/bsp/aws_libraries/3rdparty/mbedtls_config
    )

    # final binary
    target_link_libraries(keyword
        nx-ip-link-mdh
        mcu-driver-hal
        iotsdk-serial-retarget
        iotsdk-emac-factory
        mbedtls
        cmsis-rtos-api
        netxduo
    )
endif()

# Copy the result of TF-M build in the top level build directory
# Convert axf files into bin
# Sign binaries
FetchContent_GetProperties(trusted-firmware-m)
add_custom_command(
    TARGET
        keyword
    POST_BUILD
    DEPENDS
        ${CMAKE_CURRENT_BINARY_DIR}/keyword${CMAKE_EXECUTABLE_SUFFIX}

    BYPRODUCTS
        ${CMAKE_CURRENT_BINARY_DIR}/keyword_unsigned.bin
        ${CMAKE_CURRENT_BINARY_DIR}/keyword_signed.bin
        ${CMAKE_CURRENT_BINARY_DIR}/keyword_signed_update.bin
        ${CMAKE_CURRENT_BINARY_DIR}/update-digest.bin
        ${CMAKE_CURRENT_BINARY_DIR}/update-signature.bin
        ${CMAKE_CURRENT_BINARY_DIR}/update-signature.txt

    COMMAND
        ${ARM_ELF2BIN} --bin --output ${CMAKE_CURRENT_BINARY_DIR}/keyword_unsigned.bin ${CMAKE_CURRENT_BINARY_DIR}/keyword${CMAKE_EXECUTABLE_SUFFIX} --bincombined

    COMMAND
        python3 ${trusted-firmware-m_BINARY_DIR}/install/image_signing/scripts/wrapper/wrapper.py
            -v 0.0.1
            --layout ${PRJ_DIR}/bsp/signing_layout_ns.c
            -k ${trusted-firmware-m_BINARY_DIR}/install/image_signing/keys/root-RSA-2048_1.pem
            --public-key-format full
            --align 1
            --pad
            --pad-header
            -H 0x400
            -s auto
            ${CMAKE_CURRENT_BINARY_DIR}/keyword_unsigned.bin
            ${CMAKE_CURRENT_BINARY_DIR}/keyword_signed.bin

    COMMAND
        python3 ${trusted-firmware-m_BINARY_DIR}/install/image_signing/scripts/wrapper/wrapper.py
            -v 0.0.2
            --layout ${PRJ_DIR}/bsp/signing_layout_ns.c
            -k ${trusted-firmware-m_BINARY_DIR}/install/image_signing/keys/root-RSA-2048_1.pem
            --public-key-format full
            --align 1
            --pad
            --pad-header
            -H 0x400
            -s auto
            ${CMAKE_CURRENT_BINARY_DIR}/keyword_unsigned.bin
            ${CMAKE_CURRENT_BINARY_DIR}/keyword_signed_update.bin

    COMMAND
        openssl dgst -sha256 -binary -out ${CMAKE_CURRENT_BINARY_DIR}/update-digest.bin ${CMAKE_CURRENT_BINARY_DIR}/keyword_signed_update.bin

    COMMAND
        openssl pkeyutl -sign -pkeyopt digest:sha256 -pkeyopt rsa_padding_mode:pss -pkeyopt rsa_mgf1_md:sha256 -inkey ${trusted-firmware-m_BINARY_DIR}/install/image_signing/keys/root-RSA-2048_1.pem -in ${CMAKE_CURRENT_BINARY_DIR}/update-digest.bin -out ${CMAKE_CURRENT_BINARY_DIR}/update-signature.bin

    COMMAND
        openssl base64 -A -in ${CMAKE_CURRENT_BINARY_DIR}/update-signature.bin -out ${CMAKE_CURRENT_BINARY_DIR}/update-signature.txt

    COMMAND
        ${CMAKE_COMMAND} -E echo "Use this base 64 encoded signature in OTA job:"

    COMMAND
        ${CMAKE_COMMAND} -E cat ${CMAKE_CURRENT_BINARY_DIR}/update-signature.txt
)

add_test(
    NAME    keyword-test
    COMMAND pytest -s examples/keyword/tests
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    COMMAND_EXPAND_LISTS
)
