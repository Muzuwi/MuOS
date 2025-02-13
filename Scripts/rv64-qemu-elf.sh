#!/bin/bash

# Run the RV64 kernel executable in QEMU
# Requires qemu-system-riscv64 to be installed.
# Assumed work directory: <repo>/Root/

SCRIPTDIR=$(dirname "$(realpath -s "$0")")
MEMSIZE="2G"
KERNELNAME="uKernel.riscv64.uImage"
if [ -f "${KERNELNAME}" ]; then
    KERNELPATH="${PWD}/${KERNELNAME}"
elif [ -f "${SCRIPTDIR}/../Bin/${KERNELNAME}" ]; then
    KERNELPATH="${SCRIPTDIR}/../Bin/${KERNELNAME}"
else
    echo "Bootable uImage (${KERNELNAME}) not found in the current directory, nor the ../Bin/ directory (script-relative)."
    exit 1
fi

exec qemu-system-riscv64 \
  -machine sifive_u \
  -d cpu_reset,unimp,guest_errors \
  -smp 5 \
  -m "${MEMSIZE}" \
  -s \
  -S \
  -serial stdio \
  -kernel "${KERNELPATH}" \
  -no-reboot -no-shutdown
