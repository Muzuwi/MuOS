#!/usr/bin/env bash

# This script runs the kernel using QEMU, without KVM
# No-KVM is better suited for debugging, where GDB integration
# actually works properly.
# Assumed work directory: <repo>/Root/

ISONAME="MuOS.iso"
VCORES="4"
MEMSIZE="128"

if [ ! -f "${ISONAME}" ]; then
    echo "Bootable ISO (${ISONAME}) not found in the current directory."
    exit 1
fi

exec qemu-system-x86_64 \
    -smp "${VCORES}" \
    -boot d \
    -cdrom "${ISONAME}" \
    -d pcall,cpu_reset,unimp,guest_errors,page \
    -m "${MEMSIZE}" \
    -s \
    -serial stdio \
    -vga std \
    -no-reboot -no-shutdown
