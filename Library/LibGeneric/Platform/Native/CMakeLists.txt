add_library(LibGeneric
    ELFParser.cpp
    Platform/Native/Allocator-Native.cpp)

set(LIBGEN_CXX_FLAGS
    -Wall -Wextra --std=c++20
    -fpermissive)

target_compile_options(LibGeneric
    PRIVATE $<$<OR:$<COMPILE_LANGUAGE:CXX>,$<COMPILE_LANGUAGE:C>>:${LIBGEN_CXX_FLAGS}>)
