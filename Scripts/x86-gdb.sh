#!/usr/bin/env bash

SCRIPTDIR=$(dirname "$(realpath -s "$0")")
REMOTE="localhost:1234"
KERNELNAME="KernelELF.x86_64.elf"
if [ -f "${KERNELNAME}" ]; then
    KERNELPATH="${PWD}/${KERNELNAME}"
elif [ -f "${SCRIPTDIR}/../Bin/${KERNELNAME}" ]; then
    KERNELPATH="${SCRIPTDIR}/../Bin/${KERNELNAME}"
else
    echo "Kernel ELF (${KERNELNAME}) not found in the current directory, nor the ../Bin/ directory (script-relative)."
    exit 1
fi

exec gdb --tui -q \
    -ex "set confirm off" \
    -ex "set architecture i386:x86-64:intel" \
    -ex "set disassembly-flavor intel" \
    -ex "target extended-remote ${REMOTE}" \
    -ex "file ${KERNELPATH}" \
    -ex "set confirm on" \
    -ex "c"
