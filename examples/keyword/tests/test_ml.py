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


def test_ml(fvp):
    # Traces expected in the output
    expectations = [
        'Starting bootloader',
        'Booting TF-M v1.6.0',
        'Starting scheduler from ns main',
        'Ethos-U55 device initialised',
        'ML interface initialised',
        'ML_HEARD_ON',
        'ML_HEARD_OFF',
        'ML UNKNOWN',
        'ML_HEARD_GO',
        'ML UNKNOWN'
    ]

    index = 0
    start = timer()
    current_time = timer()

    # Timeout for the test is 10 minutes
    while (current_time - start) < (10 * 60):
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
