#!/bin/bash

# Run the RV64 kernel executable in Renode
# Requires Renode to be installed.
# This also runs a GDB server at :1234 that can be
# used to attach and debug the kernel.
# Assumed work directory: <repo>/Root/

SCRIPTDIR=$(dirname "$(realpath -s "$0")")
KERNELNAME="uKernel.riscv64.uImage"
if [ -f "${KERNELNAME}" ]; then
    KERNELPATH="${PWD}/${KERNELNAME}"
elif [ -f "${SCRIPTDIR}/../Bin/${KERNELNAME}" ]; then
    KERNELPATH="${SCRIPTDIR}/../Bin/${KERNELNAME}"
else
    echo "Bootable uImage (${KERNELNAME}) not found in the current directory, nor the ../Bin/ directory (script-relative)."
    exit 1
fi

if ! which renode >/dev/null 2>&1 ; then
    echo "Renode is not installed!"
    exit 1
fi

SCRIPTPATH="$( cd -- "$(dirname "$0")" >/dev/null 2>&1 ; pwd -P )"
exec renode "${SCRIPTPATH}/renode-riscv64/hifive-unleashed.resc" "$@"