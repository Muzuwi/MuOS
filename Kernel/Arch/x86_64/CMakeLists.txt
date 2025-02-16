# CMakeLists file for the x86_64 platform
cmake_minimum_required(VERSION 3.17)
project(Arch_x86_64 LANGUAGES CXX ASM_NASM)

file(GLOB_RECURSE ARCH_SOURCES *.hpp *.cpp *.asm)
add_library(Architecture OBJECT ${ARCH_SOURCES})
add_library(x86_64::LibArch ALIAS Architecture)

target_compile_options(Architecture
    PRIVATE $<$<COMPILE_LANGUAGE:CXX>:${MUOS_CXX_FLAGS}>
    PRIVATE $<$<COMPILE_LANGUAGE:ASM_NASM>:${MUOS_ASM_FLAGS}>
    PRIVATE $<$<COMPILE_LANGUAGE:ASM_NASM>:-I ${PROJECT_SOURCE_DIR}/Boot/>
    # FIXME: Once all architecture-specific code is moved out from the
    # main kernel, below flag won't be required to be public
    PUBLIC $<$<COMPILE_LANGUAGE:CXX>:-masm=intel>
    # Enable stack protector for archlib. For linking to succeed, the required
    # __stack_chk_* symbols must be provided (done in Core/Assert/StackProtector.cpp).
    PRIVATE $<$<COMPILE_LANGUAGE:CXX>:-fstack-protector-all>
    )
target_compile_definitions(Architecture
    PRIVATE ARCH_IS_x86_64=1
    )
target_include_directories(Architecture
    PRIVATE ${CMAKE_SOURCE_DIR}/Kernel/
    )
target_link_libraries(Architecture
    PRIVATE LibGeneric
    PRIVATE LibFormat
    )

# The link script flag is public, this way the kernel executable is also
# built with the proper linking layout.
target_link_options(Architecture
    PUBLIC -T ${PROJECT_SOURCE_DIR}/Linkscript_x64.ld
    )

include(MakeGrubBootable.cmake)