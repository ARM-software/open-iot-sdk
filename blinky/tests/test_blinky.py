# Copyright (c) 2021-2022, Arm Limited and Contributors. All rights reserved.
# SPDX-License-Identifier: Apache-2.0

from timeit import default_timer as timer
import pytest


@pytest.fixture
def binary_path(build_path):
    yield build_path + '/blinky/blinky_signed.bin'


def test_blinky(fvp):
    # Traces expected in the output
    expectations = [
        'Starting bootloader',
        'Booting TF-M v1.6.0',
        'Initialising kernel',
        'Starting kernel and threads',
        'The LED started blinking...',
        'LED on',
        'LED off',
        'LED on',
        'LED off',
        'LED on',
    ]

    index = 0
    start = timer()
    current_time = timer()

    # Timeout for the test is 20 seconds
    while (current_time - start) < 20:
        line = fvp.stdout.readline()
        if not line:
            break
        line = line.decode('utf-8')
        line = line.rstrip()
        print(line)
        if expectations[index] in line:
            index += 1
            if index == len(expectations):
                break
        current_time = timer()

    assert index == len(expectations)
