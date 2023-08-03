# Copyright (c) 2021-2023, Arm Limited and Contributors. All rights reserved.
# SPDX-License-Identifier: Apache-2.0

import logging
import pytest
from timeit import default_timer as timer
from pyedmgr import TestDevice


@pytest.mark.asyncio
async def test_blinky(fvp: TestDevice):
    # Traces expected in the output
    expectations = [
        "Starting bootloader",
        "Booting TF-M v1.8.0",
        "The LED started blinking...",
        "LED on",
        "LED off",
        "LED on",
        "LED off",
        "LED on",
    ]

    # Check expectations
    index = 0
    start = timer()
    current_time = timer()

    # Timeout for the test is 20 seconds
    while (current_time - start) < 20:
        line = await fvp.channel.readline_async()
        line = line.rstrip().decode("utf-8", "replace")
        if line:
            logging.info(line)
        if expectations[index] in line:
            index += 1
            if index == len(expectations):
                break
        current_time = timer()

    assert index == len(expectations)
