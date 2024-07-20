cmake_minimum_required(VERSION 3.17)
project(LibAllocator LANGUAGES CXX)

add_library(LibAllocator INTERFACE)

target_include_directories(LibAllocator
    INTERFACE
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/Include>
        $<INSTALL_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/Include>
    )

if (LIBALLOCATOR_BUILD_TESTS)
    message(STATUS "LibAllocator: building tests")
    add_subdirectory(Tests/)
endif ()
