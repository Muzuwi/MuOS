find_package(Catch2 REQUIRED)

add_library(LibGeneric
    ELFParser.cpp
    Platform/Native/Allocator-Native.cpp)

set(LIBGEN_CXX_FLAGS
    -Wall -Wextra --std=c++20
    )

target_compile_options(LibGeneric
    PRIVATE $<$<OR:$<COMPILE_LANGUAGE:CXX>,$<COMPILE_LANGUAGE:C>>:${LIBGEN_CXX_FLAGS}>)


set(TESTLIBGEN_CXX_FLAGS
    -Wall -Wextra
    --std=c++20
    -fsanitize=address -fsanitize=undefined -fsanitize=leak)
set(TESTLIBGEN_LINKER_FLAGS
    -fsanitize=address -fsanitize=undefined -fsanitize=leak)

add_executable(TestLibGeneric
    Platform/Native/Tests/Utility.cpp
    Platform/Native/Tests/Main.cpp
    Platform/Native/Tests/BitMap.cpp
    Platform/Native/Tests/List.cpp
    Platform/Native/Tests/Algorithm.cpp
    Platform/Native/Tests/Vector.cpp
    Platform/Native/Tests/SharedPtr.cpp)
target_link_libraries(TestLibGeneric PRIVATE
    Catch2::Catch2
    LibGeneric)
target_compile_options(TestLibGeneric
    PRIVATE $<$<OR:$<COMPILE_LANGUAGE:CXX>,$<COMPILE_LANGUAGE:C>>:${TESTLIBGEN_CXX_FLAGS}>)
target_link_options(TestLibGeneric
    PRIVATE $<$<OR:$<COMPILE_LANGUAGE:CXX>,$<COMPILE_LANGUAGE:C>>:${TESTLIBGEN_LINKER_FLAGS}>)
