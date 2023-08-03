#  Copyright (c) 2023 Arm Limited. All rights reserved.
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
import azure_device_update
import time
import pytest

from pytest import fixture
from timeit import default_timer as timer
from pyedmgr import TestDevice


@fixture(scope="function")
def azure_resources(build_path, credentials_path):
    try:
        # Test setup
        unique_id = None
        credentials_header = credentials_path + "/azure_iot_credentials.h"
        with open(credentials_header, "r") as file:
            for line in file:
                if "#define DEVICE_ID" in line:
                    unique_id = line.split()[2].strip('"')

        assert unique_id is not None

        resources = {
            "deployment": unique_id,
            "provider": "Arm-Ltd",
            "update": unique_id,
            "version": "0.0.2",
            "group": unique_id,
            "directory": unique_id,
        }

        manifest = build_path + "/examples/keyword/keyword-0.0.2.importmanifest.json"
        payloads = [build_path + "/examples/keyword/keyword_signed_update.bin"]
        azure_device_update.import_update(manifest, payloads, resources["directory"])

        yield resources

    finally:
        # Test teardown
        azure_device_update.cleanup_resources(
            resources["group"],
            resources["provider"],
            resources["update"],
            resources["version"],
            resources["directory"],
        )


@pytest.mark.asyncio
async def test_ota(azure_resources, fvp: TestDevice):
    async def check_output(fvp, expectations, fails):
        index = 0
        start = timer()
        current_time = timer()

        SECONDS_IN_MINUTE = 60
        while (current_time - start) < (5 * SECONDS_IN_MINUTE):
            line = await fvp.channel.readline_async()
            if not line:
                break
            line = line.decode("utf-8")
            line = line.rstrip()
            logging.info(line)
            if expectations[index] in line:
                index += 1
                if index == len(expectations):
                    break
            for x in fails:
                assert x not in line
            current_time = timer()

        assert index == len(expectations)

    # Expected output from initial firmware
    expectations = [
        "Starting bootloader",
        "Booting TF-M v1.8.0",
        "Firmware version: 0.0.1",
        "Azure Device Update agent started",
    ]

    fails = [
        "Failed to initialize iothub client",
        "Failed on nx_azure_iot_hub_client_connect",
        "Failed to start Azure Device Update agent",
        "Failed to send blink_event message to ui_msg_queue",
    ]

    check_output(fvp, expectations, fails)

    logging.info(
        "[Test host] Waiting 20 seconds for the Device Update group to be created by the cloud"
    )
    time.sleep(20)

    # Deploy the update
    azure_device_update.deploy_update(
        azure_resources["deployment"],
        azure_resources["provider"],
        azure_resources["update"],
        azure_resources["version"],
        azure_resources["group"],
    )

    # Expected output from firmware update and new firmware
    expectations = [
        "Received new update",
        "Firmware downloaded",
        "Firmware installed",
        "Starting bootloader",
        "Image upgrade secondary slot -> primary slot",
        "Firmware version: 0.0.2",
    ]

    fails = [
        "Firmware download fail",
        "Firmware verify fail",
        "ADU agent apply fail",
    ]

    await check_output(fvp, expectations, fails)
