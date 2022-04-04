#!/bin/bash

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

# Download dependencies, setup ml and apply patches.

NAME="$(basename "$0")"
HERE="$(dirname "$0")"
ROOT="$(realpath $HERE/..)"
EXAMPLE=""
CLEAN=0
export GIT_SSL_NO_VERIFY=1

function sync_submodules {
    echo "Syncronising submodules" >&2
    git submodule update --init --recursive --depth 10 --jobs 4 --progress
}

sync_submodules
