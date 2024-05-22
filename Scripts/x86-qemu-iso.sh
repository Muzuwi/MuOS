#!/usr/bin/env bash

# This script runs the bootable kernel image using QEMU
# Assumed work directory: <repo>/Root/

ISONAME="MuOS.iso"
VCORES="4"
MEMSIZE="128"

if [ ! -f "${ISONAME}" ]; then
    echo "Bootable ISO (${ISONAME}) not found in the current directory."
    exit 1
fi

exec qemu-system-x86_64 \
    -cpu host \
    -enable-kvm \
    -smp "${VCORES}" \
    -boot d \
    -cdrom "${ISONAME}" \
    -d cpu_reset,unimp,guest_errors,int \
    -m "${MEMSIZE}" \
    -s \
    -serial stdio \
    -vga std \
    -no-reboot -no-shutdown
