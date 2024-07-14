# Configures the CMake environment for building the project
# This enables the required features, sets the proper module
# search paths and provides default configuration options
# for the build.

# Enable CTest
enable_testing()

# If not explicitly specified, we probably want to build the
# kernel and not native tests.
if(NOT DEFINED MU_BUILD_TYPE)
    set(MU_BUILD_TYPE cross)
endif()

# Add custom CMake modules to the search path
list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_LIST_DIR}/)
