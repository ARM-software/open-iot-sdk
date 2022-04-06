# Copyright (c) 2021, Arm Limited and Contributors. All rights reserved.
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.21)

# Declare the blinky executable
add_executable(blinky
    "${PRJ_DIR}/blinky/freertos_adaptation.c"
    "${PRJ_DIR}/blinky/main_ns.c"
)

target_link_libraries(blinky
    startup
    lib-bsp
    tfm-ns-interface-cmsis-rtos
    freertos-kernel-heap-4
    mcu-driver-hal
    cmsis-rtos-api
    freertos-cmsis-rtos
    freertos-kernel
)

# Copy the result of TF-M build in the top level build directory
# Convert axf files into bin
# Sign binaries
ExternalProject_Get_Property(tf-m BINARY_DIR)
ExternalProject_Get_Property(tf-m SOURCE_DIR)
add_custom_command(
    TARGET
        blinky
    POST_BUILD
    DEPENDS
        ${CMAKE_CURRENT_BINARY_DIR}/blinky.axf

    BYPRODUCTS
        ${CMAKE_CURRENT_BINARY_DIR}/blinky_unsigned.bin
        ${CMAKE_CURRENT_BINARY_DIR}/blinky_signed.bin

    COMMAND
        ${ARM_ELF2BIN} --bin --output ${CMAKE_CURRENT_BINARY_DIR}/blinky_unsigned.bin ${CMAKE_CURRENT_BINARY_DIR}/blinky.axf --bincombined

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
            ${CMAKE_CURRENT_BINARY_DIR}/blinky_unsigned.bin
            ${CMAKE_CURRENT_BINARY_DIR}/blinky_signed.bin
)

add_test(
    NAME    blinky-test
    COMMAND pytest -s blinky/tests
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    COMMAND_EXPAND_LISTS
)
