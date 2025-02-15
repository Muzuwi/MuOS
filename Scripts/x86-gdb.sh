#!/usr/bin/env bash

SCRIPTDIR=$(dirname "$(realpath -s "$0")")
REMOTE="localhost:1234"
BINARY="${SCRIPTDIR}/../Root/uKernel.bin"

exec gdb --tui -q \
    -ex "set confirm off" \
    -ex "set architecture i386:x86-64:intel" \
    -ex "set disassembly-flavor intel" \
    -ex "target extended-remote ${REMOTE}" \
    -ex "file ${BINARY}" \
    -ex "set confirm on" \
    -ex "c"
