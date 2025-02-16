# CMakeLists file for the riscv64 platform
cmake_minimum_required(VERSION 3.17)
project(Arch_riscv64 LANGUAGES CXX ASM)

add_subdirectory(Boot0/)

set(ARCH_SOURCES
    Boot.s
    Platform.cpp
    SbiConsole.cpp
    VM.cpp
    )
add_library(Architecture STATIC ${ARCH_SOURCES})
add_library(riscv64::LibArch ALIAS Architecture)

target_compile_options(Architecture
    PRIVATE $<$<COMPILE_LANGUAGE:CXX>:${MUOS_CXX_FLAGS}>
    # Enable stack protector for archlib. For linking to succeed, the required
    # __stack_chk_* symbols must be provided (done in Core/Assert/StackProtector.cpp).
    PRIVATE $<$<COMPILE_LANGUAGE:CXX>:-fstack-protector-all>
    PRIVATE $<$<COMPILE_LANGUAGE:ASM>:${MUOS_ASM_FLAGS}>
    )
target_compile_definitions(Architecture
    PUBLIC ARCH_IS_RISCV64=1
    )
target_include_directories(Architecture
    PRIVATE ${CMAKE_SOURCE_DIR}/Kernel/
    )
target_link_libraries(Architecture
    PRIVATE LibFormat
    PRIVATE LibGeneric
    )
# The link script flag is public, this way the kernel executable is also
# built with the proper linking layout.
target_link_options(Architecture
    PUBLIC -T ${PROJECT_SOURCE_DIR}/Linkscript.ld
    )

include(MakeUbootBootable.cmake)