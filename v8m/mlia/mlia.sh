#!/bin/bash

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

set -e
set -o pipefail

function enable_virt_env {
    local VIRT_ENV=$1
    local UPDATE_PIP=$2
    local CHECK_VIRT_ENV=$3

    if [[ $CHECK_VIRT_ENV -eq 1 ]]; then
        if [[ ! -d "$VIRT_ENV" ]]; then
            echo "Please run next command first to build the example: ats.sh build mlia" >&2
            exit 1
        fi
    fi

    source "$VIRT_ENV/bin/activate"

    if [[ $UPDATE_PIP -eq 1 ]]; then
        pip install --upgrade pip
    fi
}

function create_and_enable_virt_env {
    local VIRT_ENV=$1
    local VIRTUALENV="$(command -v virtualenv)"

    if [[ ! -f "$VIRTUALENV" ]]; then
        pip install --user virtualenv
        export PATH="$PATH:~/.local/bin/"
    fi

    virtualenv --python python3.8 "$VIRT_ENV"
    enable_virt_env "$VIRT_ENV" 1
}

function install_mlia {
    pip install mlia==0.5.0
}

function download_models {
    local BUILD_PATH="$1"
    local MODELS_DIR="$BUILD_PATH/models"
    local DS_CNN_MODEL_URL="https://github.com/ARM-software/ML-zoo/raw/a27be36c0388380a8d2c4faea9a5713550cb091a/models/keyword_spotting/ds_cnn_large/tflite_int8/ds_cnn_l_quantized.tflite"

    mkdir -p "$MODELS_DIR"
    wget -nv "$DS_CNN_MODEL_URL" -P "$MODELS_DIR"
}

function install_backends {
    local VHT_PATH="/opt/VHT"
    local BACKENDS="Corstone-300 Corstone-310"

    local failed=""
    for backend in $BACKENDS; do
        if mlia-backend install --path "$VHT_PATH" --noninteractive $backend; then
            echo "mlia: Successfully installed backend: [$backend]"
        else
            failed="${failed}${backend},"
        fi
    done
    if [ "$failed" != "" ]; then
        echo "mlia: Failed to install backends: [${failed%,}]. Found paths: [$(ls $VHT_PATH/VHT*)]"
        exit -1
    fi
}

function prepare_build_path {
    local CLEAN=$1
    local BUILD_PATH=$2

    if [[ $CLEAN -ne 0 ]]; then
        echo "Clean building mlia" >&2
        rm -rf "$BUILD_PATH"
    elif [[ -e "$BUILD_PATH" ]]; then
        echo "Example mlia already installed. For a new installation, use the \"clean\" option: ats.sh build mlia --clean" >&2
        exit 1
    fi
}

function build_mlia {
    set -e

    local CLEAN=$1
    local BUILD_PATH=$2
    local VIRT_ENV="$BUILD_PATH/venv"

    prepare_build_path "$CLEAN" "$BUILD_PATH"
    create_and_enable_virt_env "$VIRT_ENV"
    install_mlia
    install_backends
    download_models "$BUILD_PATH"
    deactivate
}

function mlia_cmd() {
    local parameters="$1"
    local mlia_output="$2"

    local cmd="mlia $parameters"
    echo "TS: Running [$cmd]" | tee -a "$mlia_output"

    $cmd 2>&1 | tee -a "$mlia_output"
}

function run_mlia {
    set -e

    local BUILD_PATH="$1"
    local VIRT_ENV="$BUILD_PATH/venv"
    local MODELS_DIR="$BUILD_PATH/models"
    local MLIA_OUTPUT="$BUILD_PATH/mlia_output.txt"

    enable_virt_env "$VIRT_ENV" 0 1

    if [[ -f "$MLIA_OUTPUT" ]]; then
      rm "$MLIA_OUTPUT"
    fi

    local COMMON_ARGS="$MODELS_DIR/ds_cnn_l_quantized.tflite --working-dir $BUILD_PATH/mlia_output --verbose"
    mlia_cmd "operators   $COMMON_ARGS" "$MLIA_OUTPUT"
    mlia_cmd "performance $COMMON_ARGS" "$MLIA_OUTPUT"
}
