# Copyright (c) 2023, Arm Limited and Contributors. All rights reserved.
# SPDX-License-Identifier: Apache-2.0

import pytest


@pytest.fixture
def binary_path(build_path):
    yield f"{build_path}/examples/blinky/blinky_signed.bin"
