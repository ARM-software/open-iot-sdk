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

import pytest
import os
import subprocess


def pytest_addoption(parser):
    parser.addoption("--build-path", action="store", default="build")
    parser.addoption("--credentials-path", action="store", default="credentials")
    parser.addoption("--avh", action="store", default="/arm/fvp/VHT_Corstone_SSE-300_Ethos-U55")


@pytest.fixture()
def build_path(pytestconfig):
    root = os.path.dirname(os.path.abspath(__file__))
    yield root + '/' + pytestconfig.getoption("--build-path")


@pytest.fixture
def credentials_path(pytestconfig):
    root = os.path.dirname(os.path.abspath(__file__))
    yield root + '/' + pytestconfig.getoption("--credentials-path")


@pytest.fixture
def fvp_path(pytestconfig):
    yield pytestconfig.getoption("--avh")


@pytest.fixture
def vsi_script_path():
    yield os.path.dirname(os.path.abspath(__file__)) + '/lib/VHT/interface/audio/python'


@pytest.fixture(scope="function")
def fvp(fvp_path, build_path, vsi_script_path, binary_path):
    # Fixture of the FVP, when it returns, the FVP is started and
    # traces are accessible through the .stdout of the object returned.
    # When the test is terminated, the FVP subprocess is closed.
    # Note: It can take few seconds to terminate the FVP
    cmdline = [
        fvp_path,
        '-a',  f'cpu0*={build_path}/bootloader/bl2.axf',
        '--data', f'{build_path}/secure_partition/tfm_s_signed.bin@0x38000000',
        '--data', f'{binary_path}@0x28060000',
        '-C', 'core_clk.mul=200000000',
        '-C', 'mps3_board.visualisation.disable-visualisation=1',
        '-C', 'mps3_board.telnetterminal0.start_telnet=0',
        '-C', 'mps3_board.uart0.out_file=-',
        '-C', 'mps3_board.uart0.unbuffered_output=1',
        '-C', 'mps3_board.uart0.shutdown_on_eot=1',
        '-C', 'cpu0.semihosting-enable=1',
        '-C', 'mps3_board.smsc_91c111.enabled=1',
        '-C', 'mps3_board.hostbridge.userNetworking=1',
        '-C', 'mps3_board.DISABLE_GATING=1',
        '-C', 'cpu0.CFGDTCMSZ=10',
        '-C', 'cpu0.CFGITCMSZ=10',
        '-C', 'cpu0.INITNSVTOR=0x00000000',
        '-C', 'cpu0.INITSVTOR=0x10000000',
        '-V', f'{vsi_script_path}'
    ]

    proc = subprocess.Popen(cmdline, stdout=subprocess.PIPE)
    yield proc
    proc.terminate()
    proc.wait()
