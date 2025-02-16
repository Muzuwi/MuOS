cmake_minimum_required(VERSION 3.17)
# Import toolchain configuration.
find_package(MuToolchain REQUIRED)
include(${CMAKE_SOURCE_DIR}/Kernel/BuildFlags.cmake)
# Configure the languages, must be done after importing the toolchain.
project(LibGeneric LANGUAGES CXX)

if(MU_MACHINE STREQUAL host)
    # Allocator implementation for native.
    add_library(GenNativeAllocator STATIC
        Platform/Native/Allocator-Native.cpp
        )
    target_include_directories(GenNativeAllocator
        PRIVATE Include/
        PUBLIC Platform/
        )
endif()

if(NOT MU_MACHINE STREQUAL host)
    # Allocator implementation for the kernel itself, uses hmalloc.
    add_library(GenKernelAllocator STATIC
        Platform/MuOS/Allocator-KHeap.cpp
        )
    target_compile_options(GenKernelAllocator
        PRIVATE $<$<COMPILE_LANGUAGE:CXX>:${MUOS_CXX_FLAGS}>
        )
    target_include_directories(GenKernelAllocator
        PRIVATE ${CMAKE_SOURCE_DIR}/Kernel/
        PRIVATE Include/
        PUBLIC Platform/
        )
endif()

# LibGeneric target. It is generally a header-only library, but will
# require one of the above allocation impls when using certain structures
# that require allocations.
add_library(LibGeneric INTERFACE)
target_compile_options(LibGeneric
    # FIXME: Validate which flags LibGeneric actually requires and add them here.
    # For now, use the same flags as the kernel.
    INTERFACE $<$<COMPILE_LANGUAGE:CXX>:${MUOS_CXX_FLAGS}>
    )
target_include_directories(LibGeneric
    INTERFACE Include/
    )
target_link_libraries(LibGeneric
    INTERFACE $<$<STREQUAL:${MU_MACHINE},host>:GenNativeAllocator>
    INTERFACE $<$<NOT:$<STREQUAL:${MU_MACHINE},host>>:GenKernelAllocator>
    )

# Tests on host platform, can only be built using host toolchain.
if(MU_MACHINE STREQUAL host)
    # Make sure Catch2 was fetched and declared
    include(ConfigureNativeTests)

    set(TESTLIBGEN_CXX_FLAGS
        -Wall -Wextra
        --std=c++20
        -fsanitize=address -fsanitize=undefined -fsanitize=leak
        )
    set(TESTLIBGEN_LINKER_FLAGS
        -fsanitize=address -fsanitize=undefined -fsanitize=leak
        )
    add_executable(TestLibGeneric
        Tests/Algorithm.cpp
        Tests/BitMap.cpp
        Tests/Function.cpp
        Tests/List.cpp
        Tests/Main.cpp
        Tests/Optional.cpp
        Tests/SharedPtr.cpp
        Tests/StaticVector.cpp
        Tests/String.cpp
        Tests/Utility.cpp
        Tests/Vector.cpp
        )
    target_compile_options(TestLibGeneric
        PRIVATE $<$<OR:$<COMPILE_LANGUAGE:CXX>,$<COMPILE_LANGUAGE:C>>:${TESTLIBGEN_CXX_FLAGS}>
        )
    target_link_libraries(TestLibGeneric PRIVATE
        Catch2::Catch2
        LibGeneric
        )
    target_link_options(TestLibGeneric
        PRIVATE $<$<OR:$<COMPILE_LANGUAGE:CXX>,$<COMPILE_LANGUAGE:C>>:${TESTLIBGEN_LINKER_FLAGS}>
        )

    catch_discover_tests(TestLibGeneric)
endif()
