include(../Toolchain/CMakeLists.txt)
message("-- Userland library install prefix is ${CMAKE_INSTALL_PREFIX}")

add_subdirectory(CRT/)

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
)

set_target_properties(LibC PROPERTIES PREFIX "")
set_target_properties(LibC PROPERTIES OUTPUT_NAME "libC")

target_compile_options(LibC
        PRIVATE $<$<OR:$<COMPILE_LANGUAGE:CXX>,$<COMPILE_LANGUAGE:C>>:-ffreestanding -mcmodel=large -mno-red-zone>
)

add_dependencies(LibC
                 LibC_CRT0
                 LibC_CRTi
                 LibC_CRTn
                )

install(TARGETS LibC
        DESTINATION lib
)

install(DIRECTORY include/
        DESTINATION include
)

add_custom_command(TARGET LibC POST_BUILD
                   COMMAND ${CMAKE_COMMAND} --install ${CMAKE_CURRENT_BINARY_DIR})
