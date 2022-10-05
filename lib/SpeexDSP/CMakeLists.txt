# Copyright (c) 2022, Arm Limited and Contributors. All rights reserved.
# SPDX-License-Identifier: Apache-2.0

include(FetchContent)
include(ExternalProject)

FetchContent_Declare(
    speexdsp
    GIT_REPOSITORY  https://gitlab.xiph.org/xiph/speexdsp
    GIT_TAG         68311d46785be76d2a186c75578d51108bff6dfb
    GIT_PROGRESS    ON
    # Note: This prevents FetchContent_MakeAvailable() from calling
    # add_subdirectory() on the fetched repository. 
    SOURCE_SUBDIR   NONE
)

FetchContent_MakeAvailable(speexdsp)
FetchContent_GetProperties(speexdsp)

add_library(speexdsp STATIC)

target_sources(speexdsp 
    PRIVATE
        ${speexdsp_SOURCE_DIR}/libspeexdsp/fftwrap.c
        ${speexdsp_SOURCE_DIR}/libspeexdsp/filterbank.c
        ${speexdsp_SOURCE_DIR}/libspeexdsp/mdf.c
        ${speexdsp_SOURCE_DIR}/libspeexdsp/preprocess.c
        ${speexdsp_SOURCE_DIR}/libspeexdsp/kiss_fft.c
        ${speexdsp_SOURCE_DIR}/libspeexdsp/kiss_fftr.c
)

target_include_directories(speexdsp
    PUBLIC
        ${speexdsp_SOURCE_DIR}/include
        ${PRJ_DIR}/lib/speexdsp/include
        ${PRJ_DIR}/lib/speexdsp/libspeexdsp
        .
)

target_compile_definitions(speexdsp
    PUBLIC
        FIXED_POINT
        HAVE_CONFIG_H
        OS_SUPPORT_CUSTOM
)

target_compile_options(speexdsp
    PRIVATE
        -Wno-double-promotion
)
