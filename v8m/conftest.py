#  Copyright (c) 2022-2023 Arm Limited. All rights reserved.
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

from functools import reduce
import pytest
import os
import sys
import shutil
import tempfile

import pytest_asyncio

from pyedmgr import TestCase, TestCaseContext, TestDevice

sys.path.append("developer-tools/utils/cloud_helper")


def pytest_addoption(parser):
    parser.addoption("--build-path", action="store", default="build")
    parser.addoption("--credentials-path", action="store", default="credentials")
    parser.addoption(
        "--avh", action="store", default="/opt/VHT/VHT_Corstone_SSE-300_Ethos-U55"
    )
    parser.addoption("--avh-options", action="store", default="")
    parser.addoption("--audio-file", action="store", default="")


@pytest.fixture()
def build_path(pytestconfig):
    root = os.path.dirname(os.path.abspath(__file__))
    yield root + "/" + pytestconfig.getoption("--build-path")


@pytest.fixture
def credentials_path(pytestconfig):
    root = os.path.dirname(os.path.abspath(__file__))
    yield root + "/" + pytestconfig.getoption("--credentials-path")


@pytest.fixture
def fvp_path(pytestconfig):
    yield pytestconfig.getoption("--avh")


@pytest.fixture
def load_address(fvp_path: str):
    return 0x28040000


@pytest.fixture
def vsi_script_path():
    yield os.path.dirname(os.path.abspath(__file__)) + "/lib/AVH/audio"


@pytest.fixture
def fvp_options(pytestconfig):
    raw_options = pytestconfig.getoption("--avh-options")

    if raw_options == "":
        return []

    options = raw_options.split(",")

    def options_builder(options, opt):
        options.append("-C")
        options.append(opt)
        return options

    return reduce(options_builder, options, [])


@pytest.fixture
def audio_file(pytestconfig, binary_path: str):
    path = pytestconfig.getoption("--audio-file")
    if path == "":
        if "examples/keyword/keyword" in binary_path:
            return "examples/keyword/tests/test.wav"
        elif "examples/speech/speech" in binary_path:
            return "examples/speech/tests/test.wav"
    return path


@pytest.fixture
def pyedmgr_config(
    fvp_path,
    fvp_options,
    bl2_path,
    tfm_path,
    binary_path,
    vsi_script_path,
    load_address,
):
    args = [
        "-C",
        "core_clk.mul=200000000",
        "-C",
        "cpu0.semihosting-enable=1",
        "-C",
        "mps3_board.DISABLE_GATING=1",
        "-C",
        "mps3_board.hostbridge.userNetworking=1",
        "-C",
        "mps3_board.smsc_91c111.enabled=1",
        "-C",
        "mps3_board.telnetterminal0.mode=raw",
        "-C",
        "mps3_board.telnetterminal0.quiet=0",
        "-C",
        "mps3_board.telnetterminal0.start_telnet=0",
        "-C",
        "mps3_board.uart0.out_file=-",
        "-C",
        "mps3_board.uart0.shutdown_on_eot=1",
        "-C",
        "mps3_board.uart0.unbuffered_output=1",
        "-C",
        "mps3_board.visualisation.disable-visualisation=1",
        "-V",
        f"{vsi_script_path}",
    ]
    args.extend(fvp_options)

    return {
        fvp_path: {
            "firmware": [
                bl2_path,
                (tfm_path, 0x38000000),
                (binary_path, load_address),
            ],
            "args": args,
        }
    }


class CopiedFile:
    """Copy from source_path to target_path in __enter__ and delete target_path in
    __exit__. If target_path already exists in __enter__, rename it, then rename it
    back in __exit__."""

    def __init__(self, source_path: str, target_path: str):
        self.target_path = target_path
        self.source_path = source_path
        if os.path.exists(self.target_path):
            _, self.backup_path = tempfile.mkstemp()
        else:
            self.backup_path = None

    def __enter__(self):
        if self.backup_path:
            os.rename(self.target_path, self.backup_path)
        shutil.copy(self.source_path, self.target_path)

    def __exit__(self, ex_type, ex_val, tb):
        os.remove(self.target_path)
        if self.backup_path:
            os.rename(self.backup_path, self.target_path)


@pytest.mark.asyncio
@pytest_asyncio.fixture(scope="function")
async def pyedmgr_context(pyedmgr_config, audio_file) -> TestCaseContext:
    if os.path.exists(audio_file):
        with CopiedFile(audio_file, "test.wav"):
            for case in TestCase.parse(pyedmgr_config):
                async with case as context:
                    yield context
    else:
        for case in TestCase.parse(pyedmgr_config):
            async with case as context:
                yield context


@pytest.mark.asyncio
@pytest_asyncio.fixture(scope="function")
async def fvp(pyedmgr_context: TestCaseContext):
    # Assert FVP was commissioned
    assert len(pyedmgr_context.allocated_devices) == 1
    fvp: TestDevice = pyedmgr_context.allocated_devices[0]
    yield fvp


@pytest.fixture
def bl2_path(build_path):
    yield f"{build_path}/bootloader/bl2.axf"


@pytest.fixture
def tfm_path(build_path):
    yield f"{build_path}/secure_partition/tfm_s_signed.bin"
