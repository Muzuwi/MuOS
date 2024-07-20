# CMakeLists file for the riscv64 platform
cmake_minimum_required(VERSION 3.17)
project(Arch_riscv64 LANGUAGES CXX ASM)

file(GLOB_RECURSE ARCH_SOURCES *.hpp *.cpp *.asm *.s)
add_library(Architecture OBJECT ${ARCH_SOURCES})
add_library(riscv64::LibArch ALIAS Architecture)

set(ARCH_CXX_FLAGS
    ${MUOS_CXX_FLAGS}
    )
set(ARCH_ASM_FLAGS
    ${MUOS_ASM_FLAGS}
    )
target_compile_options(Architecture
    PRIVATE $<$<COMPILE_LANGUAGE:CXX>:${ARCH_CXX_FLAGS}>
    PRIVATE $<$<COMPILE_LANGUAGE:ASM>:${ARCH_ASM_FLAGS}>
    )
target_compile_definitions(Architecture
    PUBLIC ARCH_IS_RISCV64=1
    )

# The link script flag is public, this way the kernel executable is also
# built with the proper linking layout.
target_link_options(Architecture
    PUBLIC -T ${PROJECT_SOURCE_DIR}/Linkscript.ld
    )