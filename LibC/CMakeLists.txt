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
        stdio/perror.c
        sys/write.c
        sys/syscall.c
)

add_library(crt0 OBJECT
    crt0.asm
    )
set_target_properties(crt0 PROPERTIES PREFIX "")

target_compile_options(LibC
        PRIVATE $<$<OR:$<COMPILE_LANGUAGE:CXX>,$<COMPILE_LANGUAGE:C>>:-ffreestanding>
)
add_dependencies(LibC
    crt0
)

install(TARGETS LibC
        DESTINATION lib
)

install(TARGETS crt0
        OBJECTS DESTINATION lib
)

install(DIRECTORY include/
        DESTINATION include
)