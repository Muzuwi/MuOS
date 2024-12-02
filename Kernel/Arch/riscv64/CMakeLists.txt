# CMakeLists file for the riscv64 platform
cmake_minimum_required(VERSION 3.17)
project(Arch_riscv64 LANGUAGES CXX ASM)

add_subdirectory(Boot0/)

set(ARCH_SOURCES
    Boot.s
    Platform.cpp
    VM.cpp
    )
add_library(Architecture STATIC ${ARCH_SOURCES})
add_library(riscv64::LibArch ALIAS Architecture)

target_compile_options(Architecture
    PRIVATE $<$<COMPILE_LANGUAGE:CXX>:${MUOS_CXX_FLAGS}>
    PRIVATE $<$<COMPILE_LANGUAGE:ASM>:${MUOS_ASM_FLAGS}>
    )
target_compile_definitions(Architecture
    PUBLIC ARCH_IS_RISCV64=1
    )

# The link script flag is public, this way the kernel executable is also
# built with the proper linking layout.
target_link_options(Architecture
    PUBLIC -T ${PROJECT_SOURCE_DIR}/Linkscript.ld
    )