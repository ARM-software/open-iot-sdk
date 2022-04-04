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

# Build an example.

NAME="$(basename "$0")"
HERE="$(dirname "$0")"
ROOT="$(realpath $HERE/..)"
EXAMPLE=""
CLEAN=0
BUILD_PATH="$ROOT/build"
CREDENTIALS_PATH="$ROOT/bsp/default_credentials"

set -e

function build_with_cmake {
    CMAKE="$(which cmake)"
    if [[ ! -f "$CMAKE" ]]; then
        echo "${NAME}: cmake is not in PATH" >&2
        exit 1
    fi

    mkdir -p $BUILD_PATH

    if [[ $CLEAN -ne 0 ]]; then
        echo "Clean building $EXAMPLE" >&2
        rm -rf $BUILD_PATH
    else
        echo "Building $EXAMPLE" >&2
    fi

    (
        set -ex

        # Note: A bug in CMake force us to set the toolchain here
		cmake -G Ninja -S . -B $BUILD_PATH --toolchain=toolchains/toolchain-armclang.cmake -DCMAKE_SYSTEM_PROCESSOR=cortex-m55 -DAWS_CONFIG_CREDENTIALS_PATH=$CREDENTIALS_PATH
		cmake --build $BUILD_PATH --target $EXAMPLE
    )
}

function show_usage {
    cat <<EOF
Usage: $0 [options] example

Download dependencies, apply patches and build an example.

Options:
    -h,--help        Show this help
    -c,--clean       Clean build
    -p,--path        Build path
    -a,--credentials Credentials path

Examples:
    blinky
    kws
EOF
}

if [[ $# -eq 0 ]]; then
    show_usage >&2
    exit 1
fi

SHORT=a:p:,c,h
LONG=credentials:path:,clean,help
OPTS=$(getopt -n build --options $SHORT --longoptions $LONG -- "$@")

eval set -- "$OPTS"

while :
do
  case "$1" in
    -h | --help )
      show_usage
      exit 0
      ;;
    -c | --clean )
      CLEAN=1
      shift
      ;;
    -p | --path )
      BUILD_PATH=$ROOT/$2
      shift 2
      ;;
    -a | --credentials )
      CREDENTIALS_PATH=$ROOT/$2
      shift 2
      ;;
    --)
      shift;
      break
      ;;
    *)
      echo "Unexpected option: $1"
      show_usage
      exit 2
      ;;
  esac
done

case "$1" in
    kws)
        EXAMPLE="kws"
        ;;
    blinky)
        EXAMPLE="blinky"
        ;;
    *)
        echo "Missing example <kws|blinky>"
        show_usage
        exit 2
        ;;
esac

build_with_cmake
