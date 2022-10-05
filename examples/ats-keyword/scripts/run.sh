#!/bin/bash

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

HERE="$(dirname "$0")"
ROOT="$(realpath $HERE/..)"
EXAMPLE=""
BUILD_PATH="build"
TARGET="Corstone-300"
FVP_BIN=""

function show_usage {
    cat <<EOF
Usage: $0 [options] example

Run an example.

Options:
    -h,--help   Show this help
    -p,--path   Build path
    -t,--target Target to run

Examples:
    blinky
    keyword
    speech
    mlia
EOF
}

SHORT=p:,t:,h
LONG=path:,target:,help
OPTS=$(getopt -n run --options $SHORT --longoptions $LONG -- "$@")

eval set -- "$OPTS"

while :
do
  case "$1" in
    -h | --help )
      show_usage
      exit 0
      ;;
    -p | --path )
      BUILD_PATH=$2
      shift 2
      ;;
    -t | --target )
      TARGET=$2
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
    keyword | speech | blinky | mlia)
        EXAMPLE="$1"
        ;;
    *)
        echo "Usage: $0 <blinky|keyword|speech|mlia>" >&2
        exit 1
        ;;
esac

case "$TARGET" in
    Corstone-300 )
      FVP_BIN="VHT_Corstone_SSE-300_Ethos-U55"
      ;;
    Corstone-310 )
      FVP_BIN="VHT_Corstone_SSE-310"
      ;;
    *)
      echo "Invalid target <Corstone-300|Corstone-310>"
      show_usage
      exit 2
      ;;
esac

if [[ "$EXAMPLE" = "mlia" ]]; then
    source "$ROOT/$EXAMPLE/mlia.sh"
    run_mlia "$BUILD_PATH/$EXAMPLE"
    exit 0
fi

set -x

VSI_PY_PATH=$ROOT/lib/AVH/audio
OPTIONS="-V $VSI_PY_PATH -C mps3_board.visualisation.disable-visualisation=1 -C mps3_board.smsc_91c111.enabled=1 -C mps3_board.hostbridge.userNetworking=1 -C cpu0.semihosting-enable=1 -C mps3_board.telnetterminal0.start_telnet=0 -C mps3_board.uart0.out_file="-"  -C mps3_board.uart0.unbuffered_output=1 --stat  -C mps3_board.DISABLE_GATING=1 -C cpu_core.core_clk.mul=200000000"

AVH_AUDIO_FILE=$ROOT/examples/$EXAMPLE/test.wav $FVP_BIN $OPTIONS -a cpu0*="$BUILD_PATH/bootloader/bl2.axf" --data "$BUILD_PATH/secure_partition/tfm_s_signed.bin"@0x38000000 --data "$BUILD_PATH/examples/$1/$1_signed.bin"@0x28060000
