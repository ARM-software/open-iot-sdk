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

NAME="$(basename "$0")"
HERE="$(dirname "$0")"
SCRIPTS="$HERE/scripts"

function show_usage {
        cat  <<EOF
Usage: $0 <command> [command options]

Commands:
    help                        Show this help
    bootstrap,bs                Bootstrap repository
    build,b         <example>   Build <example>
    run,r           <example>   Run example
    build-n-run,br  <example>   Build and run example
    pack,p                      Create CMSIS pack
EOF
}

function _assert {
    if [[ ! $1 =~ $2 ]]; then
        show_usage >&2
        echo >&2
        echo >&2 "Bad value \"$1\""
        exit 1
    fi
}

case "$1" in
    -h|-help|--help|help)
        show_usage
        exit 0
    ;;
    bootstrap|bs)
        "$SCRIPTS/bootstrap.sh"
    ;;
    build|b)
        _assert "$2" "blinky|kws"
        "$SCRIPTS/build.sh" "${@:2}"
    ;;
    run|r)
        _assert "$2" "blinky|kws"
        "$SCRIPTS/run.sh" "${@:2}"
    ;;
    build-n-run|br)
        _assert "$2" "blinky|kws"
        "$SCRIPTS/build.sh" "${@:2}" && "$SCRIPTS/run.sh" "${@:2}"
    ;;
    pack|p)
        "$SCRIPTS/make_cmsis_pack.sh"
    ;;
    *)
        show_usage >&2
        exit 1
    ;;
esac
