
#  Copyright (c) 2021 Arm Limited. All rights reserved.
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

from timeit import default_timer as timer
from pytest import fixture
from aws_test_util import Flags, create_aws_resources, cleanup_aws_resources


@fixture(scope='function')
def aws_resources(build_path, credentials_path):
    flags = Flags(build_path, credentials_path)
    flags = create_aws_resources(flags)

    # Caller won't actually do anything with this, but we have to yield something.
    yield flags

    cleanup_aws_resources(flags)


def test_ota(aws_resources, fvp):
    # Traces expected in the output
    expectations = [
        'Starting bootloader',
        'Booting TF-M v1.6.0',
        'Starting scheduler from ns main',
        'Write certificate...',
        'Firmware version: 0.0.1',
        '[INF] network up, starting demo',
        'Found valid event handler for state transition: State=[WaitingForFileBlock], Event=[ReceivedFileBlock]',
        'Received final block of the update',
        'Image upgrade secondary slot -> primary slot',
        'Firmware version: 0.0.2',
    ]

    fails = [
        'Failed to provision device private key',
        'Failed job document content check',
        'Failed to execute state transition handler'
    ]

    index = 0
    start = timer()
    current_time = timer()

    # Timeout for the test is 15 minutes
    while (current_time - start) < (15 * 60):
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
        for x in fails:
            assert x not in line
        current_time = timer()

    assert index == len(expectations)
