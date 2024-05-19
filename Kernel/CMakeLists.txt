cmake_minimum_required(VERSION 3.17)

if (NOT MU_MACHINE)
    message(FATAL_ERROR "No target architecture was specified! Please provide via MU_MACHINE")
endif ()
message(STATUS "Compiling for machine type ${MU_MACHINE}")

# Configure the architecture-specific toolchain
find_package(MuToolchain REQUIRED)
# Enable C++/NASM support
# This MUST be done after toolchain configuration above
project(uKernel LANGUAGES CXX ASM ASM_NASM)

# Common build flags, this must be included before adding
# the architecture-specific CMakeLists
include(BuildFlags.cmake)

# Include architecture-specific modules
set(_ARCH_FOLDER "${CMAKE_CURRENT_LIST_DIR}/Arch/${MU_MACHINE}")
set(_ARCH_LISTS_FILE "${_ARCH_FOLDER}/CMakeLists.txt")
if (EXISTS "${_ARCH_LISTS_FILE}")
    # Add the architecture-specific CMakeLists
    add_subdirectory("${_ARCH_FOLDER}")
else ()
    message(FATAL_ERROR
        "Could not find the CMakeLists file for architecture ${MU_MACHINE}! "
        "Check the provided machine type, otherwise this architecture may not be "
        "currently supported. Tried looking in: ${_ARCH_LISTS_FILE}")
endif ()

# Gather kernel source files
# We also exclude the architecture folder, as that is built
# separately from the rest of the kernel.
file(GLOB_RECURSE KERNEL_SOURCES *.cpp *.hpp *.asm *.c *.h)
list(FILTER KERNEL_SOURCES EXCLUDE REGEX "Kernel/Arch/*")

# Target for the kernel executable
add_executable(KernelELF ${KERNEL_SOURCES})

# Kernel build flags
set(KERNEL_CXX_FLAGS
    ${MUOS_CXX_FLAGS}
    )
set(KERNEL_ASM_FLAGS
    ${MUOS_ASM_FLAGS}
    )
target_compile_options(KernelELF
    PRIVATE $<$<COMPILE_LANGUAGE:CXX>:${KERNEL_CXX_FLAGS}>
    PRIVATE $<$<COMPILE_LANGUAGE:ASM_NASM>:${KERNEL_ASM_FLAGS}>
    )
target_compile_definitions(KernelELF
    PRIVATE MACHINE=${MU_MACHINE}
    PRIVATE __is_kernel_build__
    )
target_link_libraries(KernelELF PRIVATE
    ${MU_MACHINE}::LibArch
    LibGeneric
    LibC
    )

add_custom_command(TARGET KernelELF
    POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_SOURCE_DIR}/Root/
    COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:KernelELF> ${CMAKE_SOURCE_DIR}/Root/uKernel.bin
    COMMENT "Copying kernel executable to Root directory ${CMAKE_SOURCE_DIR}/Root "
    )
