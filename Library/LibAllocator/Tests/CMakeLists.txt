cmake_minimum_required(VERSION 3.17)
project(TestLibAllocator LANGUAGES CXX)

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
    BumpAllocator.cpp
    ChunkAllocator.cpp
    Main.cpp
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
