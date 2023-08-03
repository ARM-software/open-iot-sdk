# Copyright (c) 2021-2023 ARM Limited. All rights reserved.
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.21)

FetchContent_GetProperties(lwip-cmsis-port)

# LWIP
# lwipcore requires the config defined by lwip-cmsis-port
target_link_libraries(lwipcore
    PRIVATE
        lwip-cmsis-port
)

target_include_directories(lwipopts
    INTERFACE
        ${CMAKE_SOURCE_DIR}/bsp/lwip-config
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

if(${AUDIO} STREQUAL "ROM")
    # convert audio clip to C file when not getting audio input from VSI
    FetchContent_GetProperties(ml-embedded-evaluation-kit)
    add_custom_target(convert-keyword-audio
        DEPENDS build-ml-embedded-evaluation-kit
        BYPRODUCTS
            ${CMAKE_CURRENT_BINARY_DIR}/InputFiles.cc
            ${CMAKE_CURRENT_BINARY_DIR}/test.cc
        # use ml-embedded-evaluation-kit's Python Virtual Environment which
        # contains dependencies for gen_audio_cpp.py
        COMMAND bash -c " \
            source ${ml-embedded-evaluation-kit_SOURCE_DIR}/resources_downloaded/env/bin/activate && \
            python3 ${ml-embedded-evaluation-kit_SOURCE_DIR}/scripts/py/gen_audio_cpp.py \
            --audio_path ${CMAKE_CURRENT_LIST_DIR}/tests \
            --source_folder_path ${CMAKE_CURRENT_BINARY_DIR} \
            --header_folder_path ${CMAKE_CURRENT_BINARY_DIR} "
        VERBATIM
        USES_TERMINAL
    )

    target_sources(keyword
        PRIVATE
            ${CMAKE_CURRENT_BINARY_DIR}/InputFiles.cc
            ${CMAKE_CURRENT_BINARY_DIR}/test.cc
    )
endif()

if (${INFERENCE_ENGINE} STREQUAL "ETHOS")
    target_compile_definitions(keyword PRIVATE USE_ETHOS)
    target_sources(keyword PRIVATE source/ethosu_platform_adaptation.c)
endif()

target_compile_definitions(keyword
    PRIVATE
        # when getting audio input from VSI
        $<$<STREQUAL:${AUDIO},VSI>:AUDIO_VSI>
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

        # audio clip when not getting audio input from VSI
        $<$<STREQUAL:${AUDIO},ROM>:${CMAKE_CURRENT_BINARY_DIR}>
)

# final binary
target_link_libraries(keyword
    ts-bsp
    startup
    ts-retarget
    iotsdk-serial-retarget
    tfm-ns-interface-cmsis-rtos
    cmsis-rtos-implementation
    ts-bsp
    ml-kit-kws
    mbed-critical

    project_options
    project_warnings
)

if(${CLOUD_CLIENT} STREQUAL "AWS")
    # Complete keyword
    target_sources(keyword
        PRIVATE
            source/aws_demo.c
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
        cmsis-rtos-api
        azure-iothub-client
        azure-provisioning-client
        cloud-credentials
    )
elseif(${CLOUD_CLIENT} STREQUAL "AZURE_NETXDUO")
    target_sources(keyword
        PRIVATE
            source/azure_netxduo_demo.c
            ${CMAKE_SOURCE_DIR}/bsp/netxduo-config/nx_azure_iot_cert.c
            ${CMAKE_SOURCE_DIR}/bsp/netxduo-config/nx_azure_iot_ciphersuites.c
            ${CMAKE_SOURCE_DIR}/bsp/netxduo-config/sample_azure_iot_embedded_sdk_connect.c
    )

    # For simplicity, extract the device ID from the credentials header and use
    # it as the unique ID for the update too.
    file(STRINGS ${APP_CONFIG_CREDENTIALS_PATH}/azure_iot_credentials.h unique_id
         REGEX "define *DEVICE_ID" LIMIT_COUNT 1)
    string(REGEX MATCH "\".*\"" unique_id "${unique_id}")
    # Remove quotation marks
    string(REGEX MATCH "[a-zA-Z0-9_\-]+" unique_id "${unique_id}")
    if("${unique_id}" STREQUAL "")
        set(model ${TS_TARGET})
    else()
        # When Azure credentials are provided, make the Device Update model unique
        # so that multiple updates can be performed in the continuous integration (CI)
        # system using the same Azure IoT Hub instance without conflicts.
        set(model ${TS_TARGET}-${unique_id})
    endif()
    target_compile_definitions(keyword
        PRIVATE
            SAMPLE_MANUFACTURER="Arm-Ltd"
            SAMPLE_MODEL="${model}"
    )

    target_include_directories(keyword
        PRIVATE
            ${APP_CONFIG_CREDENTIALS_PATH}
    )

    target_include_directories(keyword
        PRIVATE
            ${CMAKE_SOURCE_DIR}/bsp/netxduo-config
            ${CMAKE_SOURCE_DIR}/bsp/aws_libraries/3rdparty/mbedtls_config
    )

    # final binary
    target_link_libraries(keyword
        nx-ip-link-cmsis-eth
        netxduo-adu-driver-psa
        iotsdk-serial-retarget
        mbedtls
        cmsis-rtos-api
        netxduo
    )
endif()

# Copy the result of TF-M build in the top level build directory
# Convert axf files into bin
# Sign binaries
FetchContent_GetProperties(trusted-firmware-m)

make_elf_to_bin_cmd(keyword_elf_to_bin_cmd
    ${CMAKE_CURRENT_BINARY_DIR}/keyword${CMAKE_EXECUTABLE_SUFFIX}
    ${CMAKE_CURRENT_BINARY_DIR}/keyword_unsigned.bin
)

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

    COMMAND
        ${keyword_elf_to_bin_cmd}

    COMMAND
        # The primary image is padded to fill the whole slot (--pad), so that the
        # trailer area at the end of the slot is in a clean state during initial boot.
        python3 ${trusted-firmware-m_BINARY_DIR}/install/image_signing/scripts/wrapper/wrapper.py
            -v 0.0.1
            --layout ${trusted-firmware-m_BINARY_DIR}/install/image_signing/layout_files/signing_layout_ns.o
            -k ${trusted-firmware-m_BINARY_DIR}/install/image_signing/keys/root-RSA-2048_1.pem
            --public-key-format full
            --align 1
            --pad
            --pad-header
            -H 0x400
            -s auto
            --measured-boot-record
            ${CMAKE_CURRENT_BINARY_DIR}/keyword_unsigned.bin
            ${CMAKE_CURRENT_BINARY_DIR}/keyword_signed.bin

    COMMAND
        # The update image is not padded to fill the whole slot (no --pad), because
        # 1) the image to download is smaller without padding
        # 2) the trailer that keeps track of boot and update statuses should not be overwritten
        python3 ${trusted-firmware-m_BINARY_DIR}/install/image_signing/scripts/wrapper/wrapper.py
            -v 0.0.2
            --layout ${trusted-firmware-m_BINARY_DIR}/install/image_signing/layout_files/signing_layout_ns.o
            -k ${trusted-firmware-m_BINARY_DIR}/install/image_signing/keys/root-RSA-2048_1.pem
            --public-key-format full
            --align 1
            --pad-header
            -H 0x400
            -s auto
            --measured-boot-record
            ${CMAKE_CURRENT_BINARY_DIR}/keyword_unsigned.bin
            ${CMAKE_CURRENT_BINARY_DIR}/keyword_signed_update.bin
)

if(${CLOUD_CLIENT} STREQUAL "AWS")
    add_custom_command(
        TARGET
            keyword
        POST_BUILD
        DEPENDS
            ${CMAKE_CURRENT_BINARY_DIR}/keyword_signed_update.bin

        BYPRODUCTS
            ${CMAKE_CURRENT_BINARY_DIR}/update-digest.bin
            ${CMAKE_CURRENT_BINARY_DIR}/update-signature.bin
            ${CMAKE_CURRENT_BINARY_DIR}/update-signature.txt

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
elseif(${CLOUD_CLIENT} STREQUAL "AZURE_NETXDUO")
    if("${unique_id}" STREQUAL "")
        message(INFO "No Device ID found in azure_iot_credentials.h, continuing without cloud connection.")
    else()
        FetchContent_GetProperties(create-adu-import-manifest)
        add_custom_command(
            TARGET
                keyword
            POST_BUILD
            DEPENDS
                ${CMAKE_CURRENT_BINARY_DIR}/keyword_signed_update.bin
            BYPRODUCTS
                ${CMAKE_CURRENT_BINARY_DIR}/keyword-0.0.2.importmanifest.json
            COMMAND
                bash ${create-adu-import-manifest_SOURCE_DIR}/create-adu-import-manifest.sh
                -p Arm-Ltd -n ${unique_id} -v 0.0.2 -c manufacturer:Arm-Ltd
                -c model:${model} -h microsoft/swupdate:1
                -r installedCriteria:1.0
                ${CMAKE_CURRENT_BINARY_DIR}/keyword_signed_update.bin
                > ${CMAKE_CURRENT_BINARY_DIR}/keyword-0.0.2.importmanifest.json
            COMMAND
                ${CMAKE_COMMAND} -E echo "Use this import manifest in OTA job: ${CMAKE_CURRENT_BINARY_DIR}/keyword-0.0.2.importmanifest.json"
            VERBATIM
        )
    endif()
endif()
