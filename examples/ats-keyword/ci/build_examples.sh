#!/usr/bin/bash

#  Copyright (c) 2021-2022 Arm Limited. All rights reserved.
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

set -xe

HERE="$(dirname "$0")"
ROOT="$(realpath $HERE/..)"

# Note: Mix long and short option to exercise the build script.

# Build binaries without credentials
$ROOT/ats.sh build blinky --path build --target $TARGET --rtos $RTOS --endpoint $ENDPOINT
$ROOT/ats.sh build keyword -p build -t $TARGET -r $RTOS -e $ENDPOINT
$ROOT/ats.sh build speech -p build -t $TARGET -r $RTOS -e $ENDPOINT

# Copy the result into the build folder
mkdir -p "$BUILD_FOLDER"/bootloader/ "$BUILD_FOLDER"/secure_partition/ "$BUILD_FOLDER"/examples/blinky/ "$BUILD_FOLDER"/examples/keyword/ "$BUILD_FOLDER"/examples/speech/
cp build/bootloader/bl2.axf "$BUILD_FOLDER"/bootloader/bl2.axf
cp build/secure_partition/tfm_s_signed.bin "$BUILD_FOLDER"/secure_partition/tfm_s_signed.bin
cp build/examples/blinky/blinky_signed.bin "$BUILD_FOLDER"/examples/blinky/blinky_signed.bin
cp build/examples/keyword/keyword_signed.bin build/examples/keyword/keyword_signed_update.bin "$BUILD_FOLDER"/examples/keyword/
cp build/examples/speech/speech_signed.bin build/examples/speech/speech_signed_update.bin "$BUILD_FOLDER"/examples/speech/

# Build keyword with credentials
$ROOT/ats.sh build keyword -p build -a "$BUILD_FOLDER"_credentials_keyword -t $TARGET -r $RTOS -e $ENDPOINT

# Copy it into the output
mkdir -p "$BUILD_FOLDER"_cloud_keyword/bootloader/ "$BUILD_FOLDER"_cloud_keyword/secure_partition/ "$BUILD_FOLDER"_cloud_keyword/examples/blinky/ "$BUILD_FOLDER"_cloud_keyword/examples/keyword/ "$BUILD_FOLDER"_cloud_keyword/examples/speech/
cp build/bootloader/bl2.axf "$BUILD_FOLDER"_cloud_keyword/bootloader/bl2.axf
cp build/secure_partition/tfm_s_signed.bin "$BUILD_FOLDER"_cloud_keyword/secure_partition/tfm_s_signed.bin
cp build/examples/keyword/keyword_signed.bin build/examples/keyword/keyword_signed_update.bin build/examples/keyword/update-signature.txt "$BUILD_FOLDER"_cloud_keyword/examples/keyword/

# Build speech with credentials
$ROOT/ats.sh build speech -p build -a "$BUILD_FOLDER"_credentials_speech -t $TARGET -r $RTOS -e $ENDPOINT

# Copy it into the output
mkdir -p "$BUILD_FOLDER"_cloud_speech/bootloader/ "$BUILD_FOLDER"_cloud_speech/secure_partition/ "$BUILD_FOLDER"_cloud_speech/examples/blinky/ "$BUILD_FOLDER"_cloud_speech/examples/speech/ "$BUILD_FOLDER"_cloud_speech/examples/speech/
cp build/bootloader/bl2.axf "$BUILD_FOLDER"_cloud_speech/bootloader/bl2.axf
cp build/secure_partition/tfm_s_signed.bin "$BUILD_FOLDER"_cloud_speech/secure_partition/tfm_s_signed.bin
cp build/examples/speech/speech_signed.bin build/examples/speech/speech_signed_update.bin build/examples/speech/update-signature.txt "$BUILD_FOLDER"_cloud_speech/examples/speech/
