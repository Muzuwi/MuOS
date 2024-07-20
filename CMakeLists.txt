# Root CMakeLists for the OS
cmake_minimum_required(VERSION 3.17)
project(MuOS LANGUAGES NONE)

include(CMake/ConfigureCMake.cmake)

if(MU_BUILD_TYPE STREQUAL "cross")
    message(STATUS "Build type: Cross-compiled, determining cross-compiler to use")
    find_package(MuToolchain REQUIRED)
endif()

add_subdirectory(Kernel/)
add_subdirectory(Library/LibC/)
add_subdirectory(Library/LibGeneric/)
add_subdirectory(Library/LibAllocator/)
