cmake_minimum_required(VERSION 3.17)
find_package(MuToolchain REQUIRED)
project(LibC LANGUAGES C)

message("-- Userland library install prefix is ${CMAKE_INSTALL_PREFIX}")

add_library(LibC STATIC
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
set(LIBC_CXX_FLAGS
    ${MUOS_CXX_FLAGS}
    )
set(LIBC_ASM_FLAGS
    ${MUOS_ASM_FLAGS}
    )
target_compile_options(KernelELF
    PRIVATE $<$<COMPILE_LANGUAGE:CXX>:${LIBC_CXX_FLAGS}>
    PRIVATE $<$<COMPILE_LANGUAGE:ASM>:${LIBC_ASM_FLAGS}>
    PRIVATE $<$<COMPILE_LANGUAGE:ASM_NASM>:${LIBC_ASM_FLAGS}>
    )
set_target_properties(LibC PROPERTIES PREFIX "")
set_target_properties(LibC PROPERTIES OUTPUT_NAME "libC")

# Install the headers and the library to the prefix
install(TARGETS LibC
    DESTINATION lib
    )
install(DIRECTORY include/
    DESTINATION include
    )
add_custom_command(TARGET LibC POST_BUILD
    COMMAND ${CMAKE_COMMAND} --install ${CMAKE_CURRENT_BINARY_DIR})
