#  Copyright (c) 2022 Arm Limited. All rights reserved.
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

import os
import subprocess
import re


def test_ats_run_mlia(pytestconfig, build_path):
    required_patterns = [
        "ML Inference Advisor started",
        "Supported targets:",
        "Operator name",
        "Advice Generation",
        "CONV_2D",
    ]

    cmdline = f"{pytestconfig.rootpath / 'ats.sh'} run mlia"

    with subprocess.Popen(
        cmdline,
        shell=True,
        stdout=subprocess.PIPE,
        stderr=subprocess.STDOUT,
        universal_newlines=True,
    ) as proc:
        lines = proc.stdout.readlines()
        assert len(lines) > 2, f"Too few lines in output {lines}"

        for required_pattern in required_patterns:
            found = any(l for l in lines if re.search(required_pattern, l))
            assert found, f"Pattern not found {required_pattern} in output: \n {lines}"

        proc.terminate()

    with open(os.path.join(build_path, "mlia/mlia_output.txt")) as saved_output:
        assert lines == saved_output.readlines()
