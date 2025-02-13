#!/usr/bin/env bash

SCRIPTDIR=$(dirname "$(realpath -s "$0")")
REMOTE="localhost:1234"
KERNELNAME="KernelELF.riscv64.elf"
if [ -f "${KERNELNAME}" ]; then
    KERNELPATH="${PWD}/${KERNELNAME}"
elif [ -f "${SCRIPTDIR}/../Bin/${KERNELNAME}" ]; then
    KERNELPATH="${SCRIPTDIR}/../Bin/${KERNELNAME}"
else
    echo "Kernel ELF (${KERNELNAME}) not found in the current directory, nor the ../Bin/ directory (script-relative)."
    exit 1
fi

exec riscv64-elf-gdb --tui -q \
    -ex "set confirm off" \
    -ex "target extended-remote ${REMOTE}" \
    -ex "add-inferior" \
    -ex "inferior 2" \
    -ex "attach 2" \
    -ex "set schedule-multiple on" \
    -ex "i threads" \
    -ex "file ${KERNELPATH}" \
    -ex "set confirm on" \
    -ex "c"
