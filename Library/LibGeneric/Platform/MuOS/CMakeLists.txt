cmake_minimum_required(VERSION 3.17)
# Import the kernel build configuration
find_package(MuToolchain REQUIRED)
include(${CMAKE_SOURCE_DIR}/Kernel/BuildFlags.cmake)

# This compiles a shim to use LibGeneric along with the kernel allocators
# Most of the library is header-only, however structures that allocate require
# platform hooks to access the underlying system's allocator.
project(LibGenericKernelShim LANGUAGES CXX)

add_library(LibGeneric STATIC
    Platform/MuOS/Allocator-KHeap.cpp
    )
set_target_properties(LibGeneric PROPERTIES PREFIX "")
set_target_properties(LibGeneric PROPERTIES OUTPUT_NAME "libGeneric")
# Use common system build flags
target_compile_options(LibGeneric
    PRIVATE ${MUOS_CXX_FLAGS}
    )
target_compile_definitions(LibGeneric
    PRIVATE __is_kernel_build__
    )

# Install the shim to the prefix
install(TARGETS LibGeneric
    DESTINATION lib
    )
add_custom_command(TARGET LibGeneric POST_BUILD
    COMMAND ${CMAKE_COMMAND} --install ${CMAKE_CURRENT_BINARY_DIR}
    )
