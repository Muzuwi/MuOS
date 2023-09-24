# CMakeLists file for the x86_64 platform
cmake_minimum_required(VERSION 3.17)
project(Arch_x86_64 LANGUAGES CXX ASM_NASM)

file(GLOB_RECURSE ARCH_SOURCES *.hpp *.cpp *.asm)
add_library(Architecture OBJECT ${ARCH_SOURCES})
add_library(x86_64::LibArch ALIAS Architecture)

set(ARCH_CXX_FLAGS
    ${MUOS_CXX_FLAGS}
    )
set(ARCH_ASM_FLAGS
    ${MUOS_ASM_FLAGS}
    -I ${PROJECT_SOURCE_DIR}/Boot/
    )
target_compile_options(Architecture
    PRIVATE $<$<COMPILE_LANGUAGE:CXX>:${ARCH_CXX_FLAGS}>
    PRIVATE $<$<COMPILE_LANGUAGE:ASM_NASM>:${ARCH_ASM_FLAGS}>
    # FIXME: Once all architecture-specific code is moved out from the
    # main kernel, below flag won't be required to be public
    PUBLIC $<$<COMPILE_LANGUAGE:CXX>:-masm=intel>
    )
target_compile_definitions(Architecture
    PRIVATE __is_kernel_build__
    )
# The link script flag is public, this way the kernel executable is also
# built with the proper linking layout.
target_link_options(Architecture
    PUBLIC -T ${PROJECT_SOURCE_DIR}/Linkscript_x64.ld
    )