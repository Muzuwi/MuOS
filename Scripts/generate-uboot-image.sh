#!/usr/bin/env bash

TEMP=$(mktemp)

riscv64-elf-objcopy --gap-fill 0x00 -O binary uKernel.bin uKernel.flat.bin
riscv64-elf-objcopy --gap-fill 0x00 --pad-to 0x80410000 -O binary Boot0.elf Boot0.bin
cat Boot0.bin uKernel.flat.bin >"${TEMP}"

mkimage -A riscv -O linux -T kernel -C none -a 0x80400000 -e 0x80400000 -n "uKernel" -d "${TEMP}" uKernel.uImage

rm "${TEMP}"

