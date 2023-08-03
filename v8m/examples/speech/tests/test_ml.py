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
import pytest

from timeit import default_timer as timer
from pyedmgr import TestDevice


@pytest.mark.asyncio
async def test_ml(fvp: TestDevice):
    # Traces expected in the output
    expectations = [
        "Starting bootloader",
        "Booting TF-M v1.8.0",
        "ML interface initialised",
        "Init speex",
        "DSP Source",
        "Complete recognition: turn down the temperature in the bedroom",
    ]

    fails = ["Failed to send blink_event message to ui_msg_queue"]

    index = 0
    start = timer()
    current_time = timer()

    # Timeout for the test is 10 minutes
    while (current_time - start) < (10 * 60):
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
