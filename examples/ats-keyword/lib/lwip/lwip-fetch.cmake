# Copyright (c) 2021, Arm Limited and Contributors. All rights reserved.
# SPDX-License-Identifier: Apache-2.0

include(FetchContent)

FetchContent_Declare(lwip
    GIT_REPOSITORY  https://github.com/lwip-tcpip/lwip.git
    GIT_TAG         79cd89f99d1032cc5375569e5b24c375b9d230fa
    GIT_PROGRESS    ON
)

FetchContent_MakeAvailable(lwip)
FetchContent_GetProperties(lwip)
