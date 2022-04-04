# Copyright (c) 2021 ARM Limited. All rights reserved.
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.21)

add_executable(kws
    # network driver
    "${PRJ_DIR}/bsp/freertos-platform/lwip_emac_netif.c"
    "${PRJ_DIR}/bsp/freertos-platform/lwip_memory_manager.c"
    "${PRJ_DIR}/bsp/freertos-platform/lwip_stack.c"
    "${PRJ_DIR}/bsp/aws_libraries/3rdparty/lwip_osal/src/sys_arch.c"
    "${PRJ_DIR}/bsp/aws_libraries/abstractions/secure_sockets/lwip/iot_secure_sockets.c"
    "${PRJ_DIR}/bsp/aws_libraries/freertos_plus/standard/crypto/src/iot_crypto.c"
    "${PRJ_DIR}/bsp/aws_libraries/freertos_plus/standard/tls/src/iot_tls.c"

    # application
    "${PRJ_DIR}/kws/source/ml_interface.cc"
    "${PRJ_DIR}/kws/source/model_config.cc"
    "${PRJ_DIR}/kws/source/blink_task.c"
    "${PRJ_DIR}/kws/source/main_ns.c"
)

target_compile_options(kws
    PRIVATE
        $<$<COMPILE_LANGUAGE:CXX>:-std=gnu++14>
        $<$<COMPILE_LANGUAGE:C>:-std=gnu11>
)

target_compile_definitions(kws
    PRIVATE
        ARMCM55
        LIBRARY_LOG_LEVEL=3
        MBEDTLS_CONFIG_FILE=<aws_mbedtls_config.h>
        ACTIVATION_BUF_SRAM_SZ=0x00400000
        ACTIVATION_BUF_SZ=0x00400000
        ARM_MATH_DSP
        ARM_MATH_LOOPUNROLL
        ARM_MATH_MVEI
        ARM_NPU=1
        BL2
        CONFIG_KWS_SCORE_THRESHOLD=0.8
        CPU_CORTEX_M55=1
        DAUTH_CHIP_DEFAULT
        MCUBOOT_DATA_SHARING=1
        MCUBOOT_IMAGE_NUMBER=2
        MCUBOOT_SIGN_RSA
        MCUBOOT_SIGN_RSA_LEN=2048
        MPS3_PLATFORM=1
        PLATFORM_DEFAULT_UART_STDOUT=ON
        PLATFORM_HAL=1
        TFM_PARTITION_AUDIT_LOG=OFF
        TFM_PARTITION_CRYPTO
        TFM_PARTITION_FIRMWARE_UPDATE
        TFM_PARTITION_INITIAL_ATTESTATION
        TFM_PARTITION_INTERNAL_TRUSTED_STORAGE
        TFM_PARTITION_LOG_LEVEL=TFM_PARTITION_LOG_LEVEL_DEBUG
        TFM_PARTITION_PLATFORM
        TFM_PARTITION_PROTECTED_STORAGE
        TFM_PSA_API=OFF
        TFM_SPM_LOG_LEVEL=0
        TFM_SYSTEM_FP=2
        TFM_SYSTEM_FPU=2
        TFM_SYSTEM_FP_NS=2
        TF_LITE_STATIC_MEMORY
)

target_include_directories(kws
    PRIVATE
        ${PRJ_DIR}/bsp/aws_configs
        ${PRJ_DIR}/kws/source
        ${PRJ_DIR}/kws/include

        ${ml-embedded-evaluation-kit_GENERATED_DIR}/kws/include/
        ${ml-embedded-evaluation-kit_SOURCE_DIR}/dependencies/cmsis/CMSIS/DSP/Include
        ${ml-embedded-evaluation-kit_SOURCE_DIR}/dependencies/core-driver/include
        ${ml-embedded-evaluation-kit_SOURCE_DIR}/dependencies/tensorflow
        ${ml-embedded-evaluation-kit_SOURCE_DIR}/dependencies/tensorflow/tensorflow/lite/micro
        ${ml-embedded-evaluation-kit_SOURCE_DIR}/dependencies/tensorflow/tensorflow/lite/micro/tools/make/downloads/flatbuffers/include
        ${ml-embedded-evaluation-kit_SOURCE_DIR}/generated/kws/include
        ${ml-embedded-evaluation-kit_SOURCE_DIR}/source/application/hal/include
        ${ml-embedded-evaluation-kit_SOURCE_DIR}/source/application/hal/platforms/bare-metal/bsp/bsp-core/include
        ${ml-embedded-evaluation-kit_SOURCE_DIR}/source/application/hal/platforms/bare-metal/bsp/bsp-packs/mps3/include
        ${ml-embedded-evaluation-kit_SOURCE_DIR}/source/application/hal/platforms/bare-metal/bsp/include
        ${ml-embedded-evaluation-kit_SOURCE_DIR}/source/application/hal/platforms/bare-metal/timer/include
        ${ml-embedded-evaluation-kit_SOURCE_DIR}/source/application/hal/platforms/native/timer/include
        ${ml-embedded-evaluation-kit_SOURCE_DIR}/source/application/hal/platforms/native/utils/include
        ${ml-embedded-evaluation-kit_SOURCE_DIR}/source/application/main/include
        ${ml-embedded-evaluation-kit_SOURCE_DIR}/source/application/tensorflow-lite-micro/include
        ${ml-embedded-evaluation-kit_SOURCE_DIR}/source/use_case/kws/include
)

add_dependencies(kws
    ml-embedded-evaluation-kit
)

# final binary
target_link_libraries(kws
    startup
    tfm-ns-interface
    tfm-ns-interface-cmsis-rtos
    freertos-kernel-heap-4
    mcu-driver-hal
    cmsis-rtos-api
    freertos-cmsis-rtos
    freertos-kernel
    lib-bsp
    lib-AWS-client
    lwip
)

target_link_options(kws
    PRIVATE
        ${ml-embedded-evaluation-kit_LIB_DIR}/liblibethos-u-kws.a
        ${ml-embedded-evaluation-kit_LIB_DIR}/libcmsis-dsp.a
        ${ml-embedded-evaluation-kit_LIB_DIR}/libtiming_adapter.a
        ${ml-embedded-evaluation-kit_LIB_DIR}/libtensorflow-microlite.a
)

# Copy the result of TF-M build in the top level build directory
# Convert axf files into bin
# Sign binaries
ExternalProject_Get_Property(tf-m BINARY_DIR)
ExternalProject_Get_Property(tf-m SOURCE_DIR)
add_custom_command(
    TARGET
        kws
    POST_BUILD
    DEPENDS
        ${CMAKE_CURRENT_BINARY_DIR}/kws.axf

    BYPRODUCTS
        ${CMAKE_CURRENT_BINARY_DIR}/kws_unsigned.bin
        ${CMAKE_CURRENT_BINARY_DIR}/kws_signed.bin
        ${CMAKE_CURRENT_BINARY_DIR}/kws_signed_update.bin
        ${CMAKE_CURRENT_BINARY_DIR}/update-digest.bin
        ${CMAKE_CURRENT_BINARY_DIR}/update-signature.bin
        ${CMAKE_CURRENT_BINARY_DIR}/update-signature.txt

    COMMAND
        ${ARM_ELF2BIN} --bin --output ${CMAKE_CURRENT_BINARY_DIR}/kws_unsigned.bin ${CMAKE_CURRENT_BINARY_DIR}/kws.axf --bincombined

    COMMAND
        python3 ${BINARY_DIR}/install/image_signing/scripts/wrapper/wrapper.py
            -v 1.0.0
            --layout ${PRJ_DIR}/bsp/signing_layout_ns.c
            -k ${BINARY_DIR}/install/image_signing/keys/root-RSA-2048_1.pem
            --public-key-format full
            --align 1
            --pad
            --pad-header
            -H 0x400
            -s auto
            ${CMAKE_CURRENT_BINARY_DIR}/kws_unsigned.bin
            ${CMAKE_CURRENT_BINARY_DIR}/kws_signed.bin

    COMMAND
        python3 ${BINARY_DIR}/install/image_signing/scripts/wrapper/wrapper.py
            -v 1.4.0
            --layout ${PRJ_DIR}/bsp/signing_layout_ns.c
            -k ${BINARY_DIR}/install/image_signing/keys/root-RSA-2048_1.pem
            --public-key-format full
            --align 1
            --pad
            --pad-header
            -H 0x400
            -s auto
            ${CMAKE_CURRENT_BINARY_DIR}/kws_unsigned.bin
            ${CMAKE_CURRENT_BINARY_DIR}/kws_signed_update.bin

    COMMAND
        openssl dgst -sha256 -binary -out ${CMAKE_CURRENT_BINARY_DIR}/update-digest.bin ${CMAKE_CURRENT_BINARY_DIR}/kws_signed_update.bin

    COMMAND
        openssl pkeyutl -sign -pkeyopt digest:sha256 -pkeyopt rsa_padding_mode:pss -pkeyopt rsa_mgf1_md:sha256 -inkey ${BINARY_DIR}/install/image_signing/keys/root-RSA-2048_1.pem -in ${CMAKE_CURRENT_BINARY_DIR}/update-digest.bin -out ${CMAKE_CURRENT_BINARY_DIR}/update-signature.bin

    COMMAND
        openssl base64 -A -in ${CMAKE_CURRENT_BINARY_DIR}/update-signature.bin -out ${CMAKE_CURRENT_BINARY_DIR}/update-signature.txt

    COMMAND
        ${CMAKE_COMMAND} -E echo "Use this base 64 encoded signature in OTA job:"

    COMMAND
        ${CMAKE_COMMAND} -E cat ${CMAKE_CURRENT_BINARY_DIR}/update-signature.txt
)

add_test(
    NAME    kws-test
    COMMAND pytest -s kws/tests
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    COMMAND_EXPAND_LISTS
)
