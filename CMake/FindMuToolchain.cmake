cmake_minimum_required(VERSION 3.17)

# This module can be used to find a toolchain used to compile the kernel
# for a specified architecture. Set the MU_MACHINE variable to the architecture
# to find a toolchain for.

if (NOT DEFINED MU_MACHINE)
    message(FATAL_ERROR
        "MU_MACHINE is not defined! This environment variable defines the target "
        "architecture of the build, and as such is required.")
endif ()
message(STATUS "Finding toolchain for: ${MU_MACHINE}")

set(_TOOLCHAIN_DIRECTORY "${CMAKE_CURRENT_LIST_DIR}/Toolchain/${MU_MACHINE}")
set(_TOOLCHAIN_FILE "${_TOOLCHAIN_DIRECTORY}/Toolchain.cmake")

if (EXISTS "${_TOOLCHAIN_FILE}")
    message(STATUS "Using toolchain configuration file: ${_TOOLCHAIN_FILE}")
    include("${_TOOLCHAIN_FILE}")
else ()
    message(FATAL_ERROR
        "Toolchain file: ${_TOOLCHAIN_FILE} for the specified machine type: ${MU_MACHINE} "
        "does not exist! Check if the specified machine type is correct, otherwise this "
        "machine might not be compatible yet.")
endif ()

# Stop processing the current CMakeLists if the build is not using the native
# host toolchain (i.e, MU_MACHINE is different than `host`).
#
# This can be used to prevent building targets that cannot be built using the
# cross-compiler toolchain.
macro(native_build_only)
    if (NOT MU_MACHINE STREQUAL "host")
        return()
    endif ()
endmacro()

# Stop processing the current CMakeLists if the build is not using the cross
# toolchain (i.e, MU_MACHINE is equal to `host`).
#
# This can be used to prevent building targets that cannot be built using the
# native toolchain.
macro(cross_build_only)
    if (MU_MACHINE STREQUAL "host")
        return()
    endif ()
endmacro()
