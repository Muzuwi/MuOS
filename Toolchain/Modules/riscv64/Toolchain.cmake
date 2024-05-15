find_program(
    MUOS_CC
    NAMES riscv64-elf-gcc
)
find_program(
    MUOS_CXX
    NAMES riscv64-elf-g++
)

if (DEFINED MUOS_CC-NOTFOUND)
    message(FATAL_ERROR "Could not find the binary for kernel cross-compilation GCC (missing gcc binary).")
endif ()
if (DEFINED MUOS_CXX-NOTFOUND)
    message(FATAL_ERROR "Could not find the binary for kernel cross-compilation GCC (missing g++ binary).")
endif ()

# Resolve paths to the crtbegin/crtend objects
# This is necessary as we're using a cross-compilation toolchain
execute_process(COMMAND "${MUOS_CC}" -print-file-name=crtbegin.o
    OUTPUT_VARIABLE _MU_CRTBEGIN_PATH
    ERROR_QUIET
    OUTPUT_STRIP_TRAILING_WHITESPACE)
execute_process(COMMAND "${MUOS_CC}" -print-file-name=crtend.o
    OUTPUT_VARIABLE _MU_CRTEND_PATH
    ERROR_QUIET
    OUTPUT_STRIP_TRAILING_WHITESPACE)
execute_process(COMMAND "${MUOS_CC}" -print-file-name=crti.o
    OUTPUT_VARIABLE _MU_KERNEL_CRTI_OBJ
    ERROR_QUIET
    OUTPUT_STRIP_TRAILING_WHITESPACE)
execute_process(COMMAND "${MUOS_CC}" -print-file-name=crtn.o
    OUTPUT_VARIABLE _MU_KERNEL_CRTN_OBJ
    ERROR_QUIET
    OUTPUT_STRIP_TRAILING_WHITESPACE)

# Print some helpful debug info
message(STATUS "Found kernel C compiler: ${MUOS_CC}")
message(STATUS "Found kernel C++ compiler: ${MUOS_CXX}")
message(STATUS "crtbegin.o path: ${_MU_CRTBEGIN_PATH}")
message(STATUS "crtend.o path: ${_MU_CRTEND_PATH}")
message(STATUS "crti.o path: ${_MU_KERNEL_CRTI_OBJ}")
message(STATUS "crtn.o path: ${_MU_KERNEL_CRTN_OBJ}")

# These must be set
set(CMAKE_C_COMPILER ${MUOS_CC})
set(CMAKE_CXX_COMPILER ${MUOS_CXX})
set(CMAKE_ASM_COMPILER ${MUOS_NASM})
set(CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY)

# Default C/C++ flags
set(_MU_CFLAGS_DEFAULT "-fstack-protector-all -ffreestanding")
set(_MU_CXXFLAGS_DEFAULT "${_MU_CFLAGS_DEFAULT} -fno-rtti -fno-exceptions -fno-threadsafe-statics")
set(_MU_LINKFLAGS_DEFAULT "-nostdlib")

# Inject our default C/C++ flags into the compiler command line
set(CMAKE_C_COMPILE_OBJECT "<CMAKE_C_COMPILER> <DEFINES> <INCLUDES> ${_MU_CFLAGS_DEFAULT} <FLAGS> -o <OBJECT> -c <SOURCE>")
set(CMAKE_CXX_COMPILE_OBJECT "<CMAKE_CXX_COMPILER> <DEFINES> <INCLUDES> ${_MU_CXXFLAGS_DEFAULT} <FLAGS> -o <OBJECT> -c <SOURCE>")
set(_MU_OBJECT_LINK_ORDER "${_MU_KERNEL_CRTI_OBJ} ${_MU_CRTBEGIN_PATH} <OBJECTS> ${_MU_CRTEND_PATH} ${_MU_KERNEL_CRTN_OBJ} <LINK_LIBRARIES>")
set(CMAKE_CXX_LINK_EXECUTABLE "<CMAKE_CXX_COMPILER> <FLAGS> <CMAKE_CXX_LINK_FLAGS> ${_MU_LINKFLAGS_DEFAULT} <LINK_FLAGS> -Xlinker -x ${_MU_OBJECT_LINK_ORDER} -o <TARGET>")
