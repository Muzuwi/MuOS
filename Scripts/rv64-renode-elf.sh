#!/bin/bash

# Run the RV64 kernel executable in Renode
# Requires Renode to be installed.
# This also runs a GDB server at :1234 that can be
# used to attach and debug the kernel.
# Assumed work directory: <repo>/Root/

KERNELELF="uKernel.bin"

if ! which renode >/dev/null 2>&1 ; then
    echo "Renode is not installed!"
    exit 1
fi

if [ ! -f "${KERNELELF}" ]; then
    echo "Kernel executable (${KERNELELF}) not found in the current directory."
    exit 1
fi

SCRIPTPATH="$( cd -- "$(dirname "$0")" >/dev/null 2>&1 ; pwd -P )"
exec renode "${SCRIPTPATH}/renode-riscv64/hifive-unleashed.resc" "$@"