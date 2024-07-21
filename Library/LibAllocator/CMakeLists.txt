cmake_minimum_required(VERSION 3.17)
project(LibAllocator LANGUAGES CXX)

set(LIBALLOCATOR_CXX_FLAGS
    -Wall -Wextra --std=c++20
    )

add_library(LibAllocator STATIC
    Src/SlabAllocator.cpp
    Src/ChunkAllocator.cpp
    )

target_include_directories(LibAllocator
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/Include>
        $<INSTALL_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/Include>
    )

target_compile_options(LibAllocator
    PRIVATE
        ${LIBALLOCATOR_CXX_FLAGS}
    )

if (LIBALLOCATOR_BUILD_TESTS)
    message(STATUS "LibAllocator: building tests")
    add_subdirectory(Tests/)
endif ()
