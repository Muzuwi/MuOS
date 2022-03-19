include(../../Toolchain/CMakeLists.txt)

add_library(
    LibGeneric
    ELFParser.cpp
    Platform/MuOS/Allocator-KHeap.cpp)

set_target_properties(LibGeneric PROPERTIES PREFIX "")
set_target_properties(LibGeneric PROPERTIES OUTPUT_NAME "libGeneric")

target_compile_options(LibGeneric
    PRIVATE $<$<OR:$<COMPILE_LANGUAGE:CXX>,$<COMPILE_LANGUAGE:C>>:-ffreestanding -D__is_kernel_build__ -mcmodel=large -mno-red-zone>)

install(TARGETS LibGeneric
    DESTINATION lib
    )

add_custom_command(TARGET LibGeneric POST_BUILD
    COMMAND ${CMAKE_COMMAND} --install ${CMAKE_CURRENT_BINARY_DIR})
