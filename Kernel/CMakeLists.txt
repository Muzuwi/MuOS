cmake_minimum_required(VERSION 3.17)

if(NOT MU_BUILD_TYPE STREQUAL "cross")
    message(STATUS "Native tests requested - kernel build skipped")
    return()
endif()

if (NOT MU_MACHINE)
    message(FATAL_ERROR "No target architecture was specified! Please provide via MU_MACHINE")
endif ()
message(STATUS "Compiling for machine type ${MU_MACHINE}")

include(KernelSources)
# Configure the architecture-specific toolchain
find_package(MuToolchain REQUIRED)
# For binary size analytics between builds
find_package(Bloaty REQUIRED)
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

# Target for the kernel executable
add_executable(KernelELF)

# Kernel build flags
target_compile_options(KernelELF
    PRIVATE $<$<COMPILE_LANGUAGE:CXX>:${MUOS_CXX_FLAGS}>
    PRIVATE $<$<COMPILE_LANGUAGE:CXX>:-fstack-protector-all>
    PRIVATE $<$<COMPILE_LANGUAGE:ASM_NASM>:${MUOS_ASM_FLAGS}>
    )
target_compile_definitions(KernelELF
    PRIVATE MACHINE=${MU_MACHINE}
    PRIVATE ARCH_IS_${MU_MACHINE}=1
    )
target_link_libraries(KernelELF PRIVATE
    ${MU_MACHINE}::LibArch
    LibAllocator
    LibGeneric
    LibC
    )

# Include the top-level Build.cmake, which will descend down
# into the different subdirectories to collect all kernel sources.
include(Build.cmake)
target_sources(KernelELF PRIVATE ${KERNEL_SOURCES})
message(STATUS "Kernel sources: ${KERNEL_SOURCES}")

bloaty_diff(KernelELF)

add_custom_command(TARGET KernelELF
    POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_SOURCE_DIR}/Root/
    COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:KernelELF> ${CMAKE_SOURCE_DIR}/Root/uKernel.bin
    COMMENT "Copying kernel executable to Root directory ${CMAKE_SOURCE_DIR}/Root "
    )
