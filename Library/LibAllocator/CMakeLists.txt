cmake_minimum_required(VERSION 3.17)
# Import toolchain configuration.
find_package(MuToolchain REQUIRED)
include(${CMAKE_SOURCE_DIR}/Kernel/BuildFlags.cmake)
# Configure the languages, must be done after importing the toolchain.
project(LibAllocator LANGUAGES CXX)

add_library(LibAllocator STATIC
    Src/SlabAllocator.cpp
    Src/ChunkAllocator.cpp
    )
target_compile_options(LibAllocator
    # FIXME: Validate which flags LibAllocator actually requires and add them here.
    # For now, use the same flags as the kernel.
    PRIVATE $<$<COMPILE_LANGUAGE:CXX>:${MUOS_CXX_FLAGS}>
    )
target_include_directories(LibAllocator
    # On non-host, inclusion of kernel headers is required to integrate
    # with the kernel logger/panic subsystem.
    PRIVATE ${CMAKE_SOURCE_DIR}/Kernel/
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/Include>
        $<INSTALL_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/Include>
    )
target_link_libraries(LibAllocator
    # On non-host, also link LibGeneric/LibFormat to integrate with the
    # kernel's logging subsystem.
    PRIVATE $<$<NOT:$<STREQUAL:${MU_MACHINE},host>>:LibGeneric>
    PRIVATE $<$<NOT:$<STREQUAL:${MU_MACHINE},host>>:LibFormat>
    )

# Tests on host platform, can only be built using host toolchain.
if(MU_MACHINE STREQUAL host)
    # Make sure Catch2 was fetched and declared
    include(ConfigureNativeTests)

    set(TESTLIBALLOCATOR_CXX_FLAGS
        -Wall -Wextra
        --std=c++20
        -fsanitize=address -fsanitize=undefined -fsanitize=leak
        )
    set(TESTLIBALLOCATOR_LINKER_FLAGS
        -fsanitize=address -fsanitize=undefined -fsanitize=leak
        )
    add_executable(TestLibAllocator
        Tests/Bitmap.cpp
        Tests/BumpAllocator.cpp
        Tests/ChunkAllocator.cpp
        Tests/Main.cpp
        Tests/SlabAllocator.cpp
        )
    target_link_libraries(TestLibAllocator PRIVATE
        Catch2::Catch2
        LibAllocator
        )
    target_compile_options(TestLibAllocator
        PRIVATE $<$<OR:$<COMPILE_LANGUAGE:CXX>,$<COMPILE_LANGUAGE:C>>:${TESTLIBALLOCATOR_CXX_FLAGS}>
        )
    target_link_options(TestLibAllocator
        PRIVATE $<$<OR:$<COMPILE_LANGUAGE:CXX>,$<COMPILE_LANGUAGE:C>>:${TESTLIBALLOCATOR_LINKER_FLAGS}>
        )

    catch_discover_tests(TestLibAllocator)
endif()
