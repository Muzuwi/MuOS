#!/usr/bin/env bash

SCRIPTDIR=$(dirname "$(realpath -s "$0")")
REMOTE="localhost:1234"
BINARY="${SCRIPTDIR}/../Root/uKernel.bin"

exec riscv64-elf-gdb --tui -q \
    -ex "set confirm off" \
    -ex "target extended-remote ${REMOTE}" \
    -ex "add-inferior" \
    -ex "inferior 2" \
    -ex "attach 2" \
    -ex "set schedule-multiple on" \
    -ex "i threads" \
    -ex "file ${BINARY}" \
    -ex "set confirm on" \
    -ex "c"
