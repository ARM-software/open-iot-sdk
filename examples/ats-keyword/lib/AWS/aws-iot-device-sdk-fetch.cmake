# Copyright (c) 2021 ARM Limited. All rights reserved.
# SPDX-License-Identifier: Apache-2.0

include(FetchContent)

FetchContent_Declare(
  aws-iot-device-sdk

  GIT_REPOSITORY https://github.com/aws/aws-iot-device-sdk-embedded-C
  GIT_TAG        75e545b0e807ab6dff9bcb0ee5942e9a58435b10
  GIT_SHALLOW    OFF
  GIT_PROGRESS   ON

  PATCH_COMMAND  /bin/bash "${CMAKE_CURRENT_SOURCE_DIR}/patches/apply.sh"
)

FetchContent_Populate(aws-iot-device-sdk)

set(aws-iot-device-sdk_SOURCE_DIR ${aws-iot-device-sdk_SOURCE_DIR} PARENT_SCOPE)
