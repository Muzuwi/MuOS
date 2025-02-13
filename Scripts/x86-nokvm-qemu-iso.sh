#!/usr/bin/env bash

# This script runs the kernel using QEMU, without KVM
# No-KVM is better suited for debugging, where GDB integration
# actually works properly.

SCRIPTDIR=$(dirname "$(realpath -s "$0")")
ISONAME="MuOS.x86_64.iso"
if [ -f "${ISONAME}" ]; then
    ISOPATH="${PWD}/${ISONAME}"
elif [ -f "${SCRIPTDIR}/../Bin/${ISONAME}" ]; then
    ISOPATH="${SCRIPTDIR}/../Bin/${ISONAME}"
else
    echo "Bootable ISO (${ISONAME}) not found in the current directory, nor the ../Bin/ directory (script-relative)."
    exit 1
fi
VCORES="4"
MEMSIZE="128"

exec qemu-system-x86_64 \
    -smp "${VCORES}" \
    -boot d \
    -cdrom "${ISOPATH}" \
    -d pcall,cpu_reset,unimp,guest_errors,page \
    -m "${MEMSIZE}" \
    -s \
    -serial stdio \
    -vga std \
    -qmp unix:./qmp-sock,server,nowait \
    -no-reboot -no-shutdown
