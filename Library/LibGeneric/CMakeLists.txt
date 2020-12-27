include(../../Toolchain/CMakeLists.txt)

add_library(
        LibGeneric
        ELFParser.cpp
)

target_compile_options(LibGeneric
        PRIVATE $<$<OR:$<COMPILE_LANGUAGE:CXX>,$<COMPILE_LANGUAGE:C>>:-ffreestanding -fpermissive -D__is_kernel_build__ -mcmodel=large>)

install(TARGETS LibGeneric
    DESTINATION lib
    )