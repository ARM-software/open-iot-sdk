# Copyright (c) 2022-2023, Arm Limited and Contributors. All rights reserved.
# SPDX-License-Identifier: Apache-2.0

# ML-eval Kit
add_library(ml-kit INTERFACE)

# Set byproducts from ML build, so that CMake treats them as generated files
# that don't need to exist before the build.

set(ml_kit_libs
    ${ml-embedded-evaluation-kit_LIB_DIR}/libcommon_api.a
    ${ml-embedded-evaluation-kit_LIB_DIR}/libtensorflow-microlite.a
    ${ml-embedded-evaluation-kit_LIB_DIR}/libarm_math.a
    ${ml-embedded-evaluation-kit_LIB_DIR}/libcmsis-dsp.a
)

if(${INFERENCE_ENGINE} STREQUAL "ETHOS")
    list(APPEND ml_kit_libs 
        ${ml-embedded-evaluation-kit_LIB_DIR}/libethos_u_npu.a 
        ${ml-embedded-evaluation-kit_LIB_DIR}/libethosu_core_driver.a
    )

    if(${TS_TARGET} STREQUAL "Corstone-300")
        list(APPEND ml_kit_libs ${ml-embedded-evaluation-kit_LIB_DIR}/libtiming_adapter.a)
    endif()
endif()

set(ml_kit_kws_libs
    ${ml-embedded-evaluation-kit_LIB_DIR}/libkws_api.a
    ${ml-embedded-evaluation-kit_LIB_DIR}/libkws.a
)
set(ml_kit_asr_libs
    ${ml-embedded-evaluation-kit_LIB_DIR}/libasr_api.a
    ${ml-embedded-evaluation-kit_LIB_DIR}/libasr.a
)

add_custom_target(get_ml_kit_libs
    DEPENDS
        build-ml-embedded-evaluation-kit
    COMMAND
        # Dummy command that does nothing
        ${CMAKE_COMMAND} -E true
    BYPRODUCTS
        ${ml_kit_libs} ${ml_kit_kws_libs} ${ml_kit_asr_libs}
)


target_include_directories(ml-kit
    INTERFACE
        ${ml-embedded-evaluation-kit_SOURCE_DIR}/dependencies/cmsis-dsp/Include
        ${ml-embedded-evaluation-kit_SOURCE_DIR}/dependencies/core-driver/include
        ${ml-embedded-evaluation-kit_SOURCE_DIR}/dependencies/tensorflow
        ${ml-embedded-evaluation-kit_SOURCE_DIR}/dependencies/tensorflow/tensorflow/lite/micro
        ${ml-embedded-evaluation-kit_SOURCE_DIR}/dependencies/tensorflow/tensorflow/lite/micro/tools/make/downloads/flatbuffers/include
        ${ml-embedded-evaluation-kit_SOURCE_DIR}/dependencies/tensorflow/tensorflow/lite/micro/tools/make/downloads/gemmlowp/
        ${ml-embedded-evaluation-kit_SOURCE_DIR}/source/application/hal/include
        ${ml-embedded-evaluation-kit_SOURCE_DIR}/source/application/main/include
        ${ml-embedded-evaluation-kit_SOURCE_DIR}/source/application/api/common/include
        ${ml-embedded-evaluation-kit_SOURCE_DIR}/source/hal/include
        ${ml-embedded-evaluation-kit_SOURCE_DIR}/source/hal/source/platform/mps3/include
        ${ml-embedded-evaluation-kit_SOURCE_DIR}/source/log/include
        ${ml-embedded-evaluation-kit_SOURCE_DIR}/source/hal/source/components/platform_pmu/include
        ${ml-embedded-evaluation-kit_SOURCE_DIR}/source/hal/source/components/npu/include
        ${ml-embedded-evaluation-kit_SOURCE_DIR}/source/hal/source/components/stdout/include
        ${ml-embedded-evaluation-kit_SOURCE_DIR}/source/hal/source/components/lcd/include
        ${ml-embedded-evaluation-kit_SOURCE_DIR}/source/profiler/include
        ${ml-embedded-evaluation-kit_SOURCE_DIR}/source/hal/source/platform/mps3/source
        ${ml-embedded-evaluation-kit_SOURCE_DIR}/dependencies/core-platform/drivers/timing_adapter/include
        ${ml-embedded-evaluation-kit_SOURCE_DIR}/source/math/include
)

if(${TS_TARGET} STREQUAL "Corstone-300")
    target_include_directories(ml-kit
        INTERFACE
            ${ml-embedded-evaluation-kit_SOURCE_DIR}/dependencies/core-platform/drivers/timing_adapter/include
    )
endif()


target_link_libraries(ml-kit
    INTERFACE
        ${ml_kit_libs}
)

target_compile_definitions(ml-kit
    INTERFACE
        -DACTIVATION_BUF_SZ=0x00200000
        -DARM_MATH_DSP
        -DARM_MATH_LOOPUNROLL
        -DARM_MATH_MVEI
        -DARM_NPU=1
        -DTF_LITE_STATIC_MEMORY
        -DETHOS_U_NPU_MEMORY_MODE=ETHOS_U_NPU_MEMORY_MODE_SHARED_SRAM
)

add_dependencies(ml-kit
    build-ml-embedded-evaluation-kit
)


# Add kws library
add_library(ml-kit-kws INTERFACE)

target_link_libraries(ml-kit-kws
    INTERFACE
        ${ml_kit_kws_libs}
)

target_link_libraries(ml-kit-kws
    INTERFACE
        ml-kit
)

target_include_directories(ml-kit-kws
    INTERFACE
        ${ml-embedded-evaluation-kit_GENERATED_DIR}/kws/include/
        ${ml-embedded-evaluation-kit_SOURCE_DIR}/generated/kws/include
        ${ml-embedded-evaluation-kit_SOURCE_DIR}/source/use_case/kws/include
        ${ml-embedded-evaluation-kit_SOURCE_DIR}/source/application/api/use_case/kws/include
)

# Remove retarget from kws library
if(CMAKE_C_COMPILER_ID STREQUAL "GNU")
    add_custom_target(ml-kit-remove-kws-retarget
        COMMAND arm-none-eabi-ar dv ${ml-embedded-evaluation-kit_LIB_DIR}/libkws.a retarget.c.obj
    )
elseif(CMAKE_C_COMPILER_ID STREQUAL "ARMClang")
    add_custom_target(ml-kit-remove-kws-retarget
        COMMAND armar -d ${ml-embedded-evaluation-kit_LIB_DIR}/libkws.a retarget.o
    )
endif()

add_dependencies(ml-kit-remove-kws-retarget ml-kit)
add_dependencies(ml-kit-kws ml-kit-remove-kws-retarget)


# Add asr library
add_library(ml-kit-asr INTERFACE)

target_link_libraries(ml-kit-asr
    INTERFACE
        ${ml_kit_asr_libs}
)

target_link_libraries(ml-kit-asr
    INTERFACE
        ml-kit
)

target_include_directories(ml-kit-asr
    INTERFACE
        ${ml-embedded-evaluation-kit_GENERATED_DIR}/asr/include/
        ${ml-embedded-evaluation-kit_SOURCE_DIR}/generated/asr/include
        ${ml-embedded-evaluation-kit_SOURCE_DIR}/source/use_case/asr/include
        ${ml-embedded-evaluation-kit_SOURCE_DIR}/source/application/api/use_case/asr/include
)

if(CMAKE_C_COMPILER_ID STREQUAL "GNU")
    add_custom_target(ml-kit-remove-asr-retarget
        COMMAND arm-none-eabi-ar dv ${ml-embedded-evaluation-kit_LIB_DIR}/libasr.a retarget.c.obj
    )
elseif(CMAKE_C_COMPILER_ID STREQUAL "ARMClang")
    add_custom_target(ml-kit-remove-asr-retarget
        COMMAND armar -d ${ml-embedded-evaluation-kit_LIB_DIR}/libasr.a retarget.o
    )
endif()

add_dependencies(ml-kit-remove-asr-retarget ml-kit)
add_dependencies(ml-kit-asr ml-kit-remove-asr-retarget)
