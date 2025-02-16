# Configures the CMake environment for building the project
# This enables the required features, sets the proper module
# search paths and provides default configuration options
# for the build.

# Enable CTest
enable_testing()

# If MU_MACHINE is unset, default to using the host toolchain.
# This won't allow you to build the kernel itself, but it can
# be used to run native library tests on the current host.
if(NOT DEFINED MU_MACHINE)
    message(STATUS "MU_MACHINE was not provided, defaulting to host. Only library tests will be available!")
    set(MU_MACHINE host)
endif()

# Add custom CMake modules to the search path
list(APPEND CMAKE_MODULE_PATH
        ${CMAKE_CURRENT_LIST_DIR}/
        ${CMAKE_CURRENT_LIST_DIR}/Toolchain/
        )
