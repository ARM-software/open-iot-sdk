#  Copyright (c) 2021-2023 Arm Limited. All rights reserved.
#  SPDX-License-Identifier: Apache-2.0
#
#  Licensed under the Apache License, Version 2.0 (the "License");
#  you may not use this file except in compliance with the License.
#  You may obtain a copy of the License at
#
#      http://www.apache.org/licenses/LICENSE-2.0
#
#  Unless required by applicable law or agreed to in writing, software
#  distributed under the License is distributed on an "AS IS" BASIS,
#  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
#  See the License for the specific language governing permissions and
#  limitations under the License.

import logging

from timeit import default_timer as timer
from pytest import fixture, mark
from aws_test_util import Flags, create_aws_resources, cleanup_aws_resources
from pyedmgr import TestDevice

SECONDS_IN_MINUTE = 60


@fixture(scope="function")
def aws_resources(build_path, credentials_path):
    flags = Flags(build_path, credentials_path)
    flags = create_aws_resources(flags)
    try:
        # Caller won't actually do anything with this, but we have to yield something.
        yield flags
    finally:
        cleanup_aws_resources(flags)


@mark.asyncio
async def test_ota(aws_resources, fvp: TestDevice):
    # Traces expected in the output
    expectations = [
        "Starting bootloader",
        "Booting TF-M v1.8.0",
        "Write certificate...",
        "Firmware version: 0.0.1",
        "[INF] network up, starting demo",
        "Received valid file block: Block index=0, Size=4096",
        "Received final block of the update",
        "Image upgrade secondary slot -> primary slot",
        "Firmware version: 0.0.2",
    ]

    fails = [
        "Failed to provision device private key",
        "Failed job document content check",
        "Failed to send blink_event message to ui_msg_queue",
    ]

    index = 0
    start = timer()
    current_time = timer()

    while (current_time - start) < (30 * SECONDS_IN_MINUTE):
        line = await fvp.channel.readline_async()
        line = line.decode("utf-8").rstrip()
        if line:
            logging.info(line)
        if expectations[index] in line:
            index += 1
            if index == len(expectations):
                break
        for x in fails:
            assert x not in line
        current_time = timer()

    assert index == len(expectations)
