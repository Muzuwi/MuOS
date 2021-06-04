#!/bin/bash

cmake -DCMAKE_BUILD_TYPE=Debug \
      -DCMAKE_C_COMPILER=/usr/local/muOS/bin/x86_64-muos-gcc \
      -DCMAKE_CXX_COMPILER=/usr/local/muOS/bin/x86_64-muos-g++ \
      -DCMAKE_TRY_COMPILE_TARGET_TYPE=STATIC_LIBRARY \
      -DCMAKE_ASM-ATT_COMPILER=/usr/local/muOS/bin/x86_64-muos-as \
      -G "CodeBlocks - Unix Makefiles" \
      ../
