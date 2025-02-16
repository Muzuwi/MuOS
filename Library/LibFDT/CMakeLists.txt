cmake_minimum_required(VERSION 3.17)
# Import toolchain configuration.
find_package(MuToolchain REQUIRED)
include(${CMAKE_SOURCE_DIR}/Kernel/BuildFlags.cmake)
# Configure the languages, must be done after importing the toolchain.
project(LibFDT LANGUAGES CXX)

add_library(LibFDT STATIC
    DeviceTree.cpp
    )
target_compile_options(LibFDT
    # FIXME: Validate which flags LibFormat actually requires and add them here.
    # For now, use the same flags as the kernel.
    INTERFACE $<$<COMPILE_LANGUAGE:CXX>:${MUOS_CXX_FLAGS}>
    )
target_include_directories(LibFDT
    PUBLIC Include/
    )
