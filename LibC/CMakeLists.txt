include(../Toolchain/CMakeLists.txt)

add_library(
        LibC
        icxxabi.c
        stdio/printf.c
        stdio/putchar.c
        stdio/puts.c
        string/memchr.c
        string/memcmp.c
        string/memcpy.c
        string/memset.c
        string/strcmp.c
        string/strlen.c
)

target_compile_options(LibC
        PRIVATE $<$<OR:$<COMPILE_LANGUAGE:CXX>,$<COMPILE_LANGUAGE:C>>:-ffreestanding>)