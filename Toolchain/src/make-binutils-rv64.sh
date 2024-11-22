#!/bin/bash
set -e

PREFIX="/usr/local/muOS/"
JOBCOUNT=$(nproc --all)

mkdir -p build-binutils
cd build-binutils
../binutils/configure --target=riscv64-elf \
                      --prefix=$PREFIX \
                      --with-sysroot=$PREFIX \
                      --disable-nls \
                      --disable-werror \
                      --enable-shared
make -j"$JOBCOUNT"
make install
