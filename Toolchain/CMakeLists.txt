#       This file contains configuration for the Toolchain, and should be included
#       by other CMakeLists in the project

set(CMAKE_SYSTEM_NAME Generic)
set(CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY)

#  Default C/C++ flags
set(MU_CFLAGS_DEFAULT "-g -Wall -Wextra -masm=intel -mno-red-zone -fanalyzer -fdump-analyzer-callgraph")
set(MU_CXXFLAGS_DEFAULT "${MU_CFLAGS_DEFAULT} -fno-rtti -fno-exceptions -fno-threadsafe-statics -mno-red-zone")
set(MU_LINKER_FLAGS "-nostdlib -T ${CMAKE_SOURCE_DIR}/Kernel/Arch/x86_64/Linkscript_x64.ld -z max-page-size=0x1000")

#  Include dirs
include_directories(${CMAKE_SOURCE_DIR}/)
include_directories(${CMAKE_SOURCE_DIR}/Library)
include_directories(${CMAKE_SOURCE_DIR}/LibC/include)
include_directories(${CMAKE_SOURCE_DIR}/Kernel/)

enable_language(C)
set(CMAKE_C_COMPILE_OBJECT
        "<CMAKE_C_COMPILER> <DEFINES> <INCLUDES> ${MU_CFLAGS_DEFAULT} <FLAGS> -o <OBJECT> -c <SOURCE>"
        )

enable_language(CXX)
set(CMAKE_CXX_COMPILE_OBJECT
        "<CMAKE_CXX_COMPILER> <DEFINES> <INCLUDES> ${MU_CXXFLAGS_DEFAULT} <FLAGS> -o <OBJECT> -c <SOURCE>"
        )

#  Use NASM for .asm and .nasm files
set(CMAKE_ASM_NASM_OBJECT_FORMAT elf64)
set(CMAKE_ASM_NASM_SOURCE_FILE_EXTENSIONS "asm;nasm")
enable_language(ASM_NASM)

#  Use AS for .s/.S files
set(CMAKE_ASM-ATT_SOURCE_FILE_EXTENSIONS "s;S")
enable_language(ASM-ATT)
