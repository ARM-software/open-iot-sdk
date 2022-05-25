# Copyright (c) 2021-2022, Arm Limited and Contributors. All rights reserved.
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.21)

# Declare the blinky executable
add_executable(blinky main.c)

target_link_libraries(blinky
    mcu-driver-hal
    mcu-driver-bootstrap
    mdh-reference-platforms-for-arm

    cmsis-rtos-implementation

    tfm-ns-interface-cmsis-rtos

    lib-bsp
    startup
)

# Copy the result of TF-M build in the top level build directory
# Convert axf files into bin
# Sign binaries
FetchContent_GetProperties(tf-m)

add_custom_command(
    TARGET
        blinky
    POST_BUILD
    DEPENDS
        ${CMAKE_CURRENT_BINARY_DIR}/blinky${CMAKE_EXECUTABLE_SUFFIX}

    BYPRODUCTS
        ${CMAKE_CURRENT_BINARY_DIR}/blinky_unsigned.bin
        ${CMAKE_CURRENT_BINARY_DIR}/blinky_signed.bin

    COMMAND
        ${ARM_ELF2BIN} --bin --output ${CMAKE_CURRENT_BINARY_DIR}/blinky_unsigned.bin ${CMAKE_CURRENT_BINARY_DIR}/blinky${CMAKE_EXECUTABLE_SUFFIX} --bincombined

    COMMAND
        python3 ${tf-m_BINARY_DIR}/install/image_signing/scripts/wrapper/wrapper.py
            -v 0.0.1
            --layout ${PRJ_DIR}/bsp/signing_layout_ns.c
            -k ${tf-m_BINARY_DIR}/install/image_signing/keys/root-RSA-2048_1.pem
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
