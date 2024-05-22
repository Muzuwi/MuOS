#!/bin/bash

# Run the RV64 kernel executable in QEMU
# Requires qemu-system-riscv64 to be installed.
# Assumed work directory: <repo>/Root/

KERNELELF="uKernel.bin"
MEMSIZE="2G"

if [ ! -f "${KERNELELF}" ]; then
  echo "Kernel executable (${KERNELELF}) not found in the current directory."
  exit 1
fi

exec qemu-system-riscv64 \
  -machine sifive_u \
  -d cpu_reset,unimp,guest_errors \
  -smp 5 \
  -m "${MEMSIZE}" \
  -s \
  -serial stdio \
  -kernel "${KERNELELF}" \
  -no-reboot -no-shutdown
