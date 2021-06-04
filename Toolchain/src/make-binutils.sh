#!/bin/bash
set -e

PREFIX="/usr/local/muOS/"

mkdir build-binutils
cd build-binutils
../binutils/configure --target=x86_64-muos \
                      --prefix=$PREFIX \
                      --with-sysroot=$PREFIX \
                      --disable-nls \
                      --disable-werror \
                      --enable-shared
make -j12
make install
