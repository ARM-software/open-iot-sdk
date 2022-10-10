# Copyright (c) 2022, Arm Limited and Contributors. All rights reserved.
# SPDX-License-Identifier: Apache-2.0

add_library(bsp-an386
    STATIC
        bsp.c
        startup_ARMCM4.c
)
target_include_directories(bsp-an386 PUBLIC .)
target_compile_definitions(bsp-an386 PRIVATE ARMCM4_FP)
target_link_libraries(bsp-an386
    PRIVATE
        mcu-driver-hal
        cmsis-core
)

if(CMAKE_C_COMPILER_ID STREQUAL "GNU")
    set(bsp_an386_linker_script ${CMAKE_CURRENT_SOURCE_DIR}/linker.ld PARENT_SCOPE)
elseif(CMAKE_C_COMPILER_ID STREQUAL "ARMClang")
    set(bsp_an386_linker_script ${CMAKE_CURRENT_SOURCE_DIR}/linker.sct PARENT_SCOPE)
endif()
