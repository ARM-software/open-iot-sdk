# Copyright (c) 2022 ARM Limited. All rights reserved.
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.21)

set(supportedPlatforms ARM_AN552_MPS3 ARM_AN386_MPS2)
set(MDH_PLATFORM ARM_AN552_MPS3 CACHE STRING "Platform to build for.")
set_property(CACHE MDH_PLATFORM PROPERTY STRINGS ${supportedPlatforms})
if(NOT ${MDH_PLATFORM} IN_LIST supportedPlatforms)
    message(FATAL_ERROR "MDH_PLATFORM must be in ${supportedPlatforms}")
endif()

include(ExternalProject)
include(FetchContent)

# CMake processes toolchain file during first call to project(), so it needs to exist first
FetchContent_Declare(iotsdk-toolchains
    GIT_REPOSITORY  https://git.gitlab.arm.com/iot/open-iot-sdk/toolchain.git
    GIT_TAG         126d2092a2325c77fdf36043e931293c62e2ad29
    SOURCE_DIR      ${CMAKE_BINARY_DIR}/toolchains
)
FetchContent_MakeAvailable(iotsdk-toolchains)

project(blinky LANGUAGES C CXX)
set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_C_STANDARD 99)

# Declare the Open IoT SDK
FetchContent_Declare(
  open_iot_sdk
  GIT_REPOSITORY https://git.gitlab.arm.com/iot/open-iot-sdk/sdk.git
  GIT_TAG        bf11e35384e87f055ba2932f03eab09253186dcb
)
set(open_iot_sdk_POPULATED TRUE)

# Declare the Open IoT SDK configuration flags
set(IOTSDK_EXAMPLES OFF)
set(IOTSDK_FREERTOS ON)
set(IOTSDK_CMSIS_DEVICE ON)
set(IOTSDK_MDH_ARM ON)
set(FETCHCONTENT_QUIET OFF)

# Set FVP variant
set(VARIANT "FVP")

# Make the Open IoT SDK available
FetchContent_MakeAvailable(open_iot_sdk)

# Fetch project_options
FetchContent_Declare(project_options
    GIT_REPOSITORY  https://github.com/cpp-best-practices/project_options.git
    GIT_TAG         v0.21.0
)
FetchContent_MakeAvailable(project_options)
include(${project_options_SOURCE_DIR}/Index.cmake)
include(${project_options_SOURCE_DIR}/src/DynamicProjectOptions.cmake)

include(${open_iot_sdk_SOURCE_DIR}/cmake/warning_flags.cmake)
set(GCC_WARNINGS ${IOTSDK_WARNING_FLAGS} ${IOTSDK_WARNING_AS_ERRORS_FLAGS})
set(CLANG_WARNINGS ${GCC_WARNINGS})

dynamic_project_options(
    CLANG_WARNINGS
    ${CLANG_WARNINGS}
    GCC_WARNINGS
    ${GCC_WARNINGS}
)

add_subdirectory(bsp/an552 EXCLUDE_FROM_ALL)
add_subdirectory(bsp/an386 EXCLUDE_FROM_ALL)

if (MDH_PLATFORM STREQUAL ARM_AN552_MPS3)
    add_library(bsp ALIAS bsp-an552)
    set(linker_script ${bsp_an552_linker_script})
elseif (MDH_PLATFORM STREQUAL ARM_AN386_MPS2)
    add_library(bsp ALIAS bsp-an386)
    set(linker_script ${bsp_an386_linker_script})
else()
    message(FATAL_ERROR "A valid target platform must be defined, was: " ${MDH_PLATFORM})
endif()


# Compilation Database
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
add_custom_target(database COMMAND ${CMAKE_COMMAND} -E copy_if_different "${INT_DIR}/compile_commands.json" "${CMAKE_BINARY_DIR}")

add_executable(blinky src/main.c)

target_link_libraries(blinky
    bsp

    mcu-driver-hal
    freertos-kernel-heap-3
    freertos-kernel

    project_options
    project_warnings
)

target_include_directories(blinky
    PRIVATE
        freertos-config
)

if(CMAKE_C_COMPILER_ID STREQUAL "GNU")
    target_link_options(blinky PRIVATE -T ${linker_script})
elseif(CMAKE_C_COMPILER_ID STREQUAL "ARMClang")
    target_link_options(blinky PRIVATE --scatter=${linker_script})
endif()
set_target_properties(blinky PROPERTIES LINK_DEPENDS ${linker_script})


add_test(
    NAME    blinky
    COMMAND htrun
        --image-path=blinky
        --compare-log=${CMAKE_CURRENT_LIST_DIR}/test.log
        ${IOTSDK_HTRUN_COMMON_ARGUMENTS}
        ${IOTSDK_HTRUN_ARGUMENTS}
    COMMAND_EXPAND_LISTS
)
