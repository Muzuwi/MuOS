#!/bin/bash
set -e

PREFIX="/usr/local/muOS"
PATH=$PATH:$PREFIX
JOBCOUNT=$(nproc --all)
export PREFIX

if [ ! -d gcc ]; then
    echo "Extracted GCC sources not found!"
    exit 1
fi

if [ ! -d build-gcc ]; then
    echo "Creating build-dir for GCC"
    mkdir build-gcc
fi

mkdir -p -v /usr/local/muOS/usr/include/
mkdir -p -v /usr/local/muOS/include/

cd build-gcc
../gcc/configure --target=riscv64-elf \
				 --prefix="$PREFIX" \
				 --with-sysroot="$PREFIX" \
				 --disable-nls\
				 --enable-languages=c,c++ \
				 --enable-shared

make all-gcc -j"$JOBCOUNT"
make all-target-libgcc -j"$JOBCOUNT" CFLAGS_FOR_TARGET='-mcmodel=medany -march=rv64imac -mabi=lp64'
make install-gcc
make install-target-libgcc
