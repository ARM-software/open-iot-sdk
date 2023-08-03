#!/bin/bash
# Copyright (c) 2023, Arm Limited and Contributors. All rights reserved.
# SPDX-License-Identifier: Apache-2.0

HERE="$(dirname "$BASH_SOURCE")"
NAME="$(basename "$BASH_SOURCE")"
ROOT="$(realpath $HERE/..)"

[[ -z ${PYEDMGR_ROOT} ]] && PYEDMGR_ROOT="$ROOT/build/_deps/pyedmgr-src" || true

function show_usage {
    cat <<EOF
Usage: $BASH_SOURCE <example> [test case]

Example:
    blinky
    keyword
    speech

Test case (if example is keyword or speech):
    azure
    ml
    aws_ota
    azure_netxduo_ota

If no test case is given, run all test cases for example.
EOF
}

function die_usage {
    show_usage >&2
    [[ $# -gt 0 ]] && echo "$@" >&2
    exit 1
}

function have_pyedmgr {
    python -c "import pyedmgr" >/dev/null 2>&1
}

function install_pyedmgr {
    if [[ ! -d $PYEDMGR_ROOT ]]; then
        echo "$NAME: warning: could not find pyedmgr in build directory, please install it manually" >&2
        exit 1
    fi

    (
        cd "$PYEDMGR_ROOT"
        pip install -r requirements.txt
        python setup.py install
    )
}

# Enter virtual environment if not already set. Create venv if one doesn't exist.
if [[ ! -d $VIRTUAL_ENV ]]; then
    echo "$NAME: error: test script should run in a python virtual environment." >&2
    exit 1
fi

# Make sure pyedmgr and dependencies are installed
have_pyedmgr || install_pyedmgr || exit 1

declare -a cases
declare -a args
example=$1
param=""
shift

# Capture the cases to process
while [[ -n $1 ]] && [[ ! ($1 =~ -.+) ]]; do
    cases+=($1)
    shift
done

# capture remaining arguments to forward to pytest
args=( "$@" )

if [[ $example = blinky ]]; then
    pytest $ROOT/examples/blinky/tests/test_blinky.py "${args[@]}"
elif [[ $example = keyword || $example = speech ]]; then
    if [[ -z ${cases[@]} ]]; then
        cases+=(azure)
        cases+=(ml)
        cases+=(aws_ota)
        cases+=(azure_netxduo_ota)
    fi

    for case in ${cases[@]}; do
        path="$ROOT/examples/$example/tests/test_$case.py"
        if [[ -f $path ]]; then
            pytest "$path" "${args[@]}"
        else
            die_usage "No such file: $path"
        fi
    done
else
    die_usage
fi
