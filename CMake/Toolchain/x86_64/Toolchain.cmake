# Finds the toolchain for x86_64 machine builds

# Find the C/C++ compilers to use for cross-compiling the kernel
# Eventually, it'd also be good for this to allow easy compilation
# of userland applications using a simple CMakeLists.
find_program(
    MUOS_CC
    NAMES x86_64-muos-gcc
    PATHS /usr/local/muOS/bin
)
find_program(
    MUOS_CXX
    NAMES x86_64-muos-g++
    PATHS /usr/local/muOS/bin
)
find_program(
    MUOS_NASM
    NAMES nasm
)

if (DEFINED MUOS_CC-NOTFOUND)
    message(FATAL_ERROR "Could not find the binary for kernel cross-compilation GCC (missing gcc binary).")
endif ()
if (DEFINED MUOS_CXX-NOTFOUND)
    message(FATAL_ERROR "Could not find the binary for kernel cross-compilation GCC (missing g++ binary).")
endif ()
if (DEFINED MUOS_NASM-NOTFOUND)
    message(FATAL_ERROR "Could not find the binary for NASM.")
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

# Source file paths and object output paths for the kernel CRT stuff.
set(_MU_KERNEL_CRTI_SRC "${CMAKE_CURRENT_LIST_DIR}/crti.asm")
set(_MU_KERNEL_CRTI_OBJ "${CMAKE_BINARY_DIR}/ukernel_crti.o")
set(_MU_KERNEL_CRTN_SRC "${CMAKE_CURRENT_LIST_DIR}/crtn.asm")
set(_MU_KERNEL_CRTN_OBJ "${CMAKE_BINARY_DIR}/ukernel_crtn.o")

# Build the crti/crtn objects
# These are kernel-specific, and userland should probably use
# different ones.
if (NOT EXISTS "${_MU_KERNEL_CRTI_OBJ}")
    message(STATUS "Building kernel crti.o..")
    execute_process(COMMAND "${MUOS_NASM}" -f elf64 -o "${_MU_KERNEL_CRTI_OBJ}" "${_MU_KERNEL_CRTI_SRC}"
        RESULT_VARIABLE _CRTI_RET)
    if (_CRTI_RET AND NOT _CRTI_RET EQUAL 0)
        message(FATAL_ERROR "Failed building the kernel crti.o object!")
    endif ()
else ()
    message(STATUS "Kernel crti.o object already exists: ${_MU_KERNEL_CRTI_OBJ}, skipping generation..")
endif ()

if (NOT EXISTS "${_MU_KERNEL_CRTN_OBJ}")
    message(STATUS "Building kernel crtn.o..")
    execute_process(COMMAND "${MUOS_NASM}" -f elf64 -o "${_MU_KERNEL_CRTN_OBJ}" "${_MU_KERNEL_CRTN_SRC}"
        RESULT_VARIABLE _CRTN_RET)
    if (_CRTN_RET AND NOT _CRTN_RET EQUAL 0)
        message(FATAL_ERROR "Failed building the kernel crtn.o object!")
    endif ()
else ()
    message(STATUS "Kernel crtn.o object already exists: ${_MU_KERNEL_CRTN_OBJ}, skipping generation..")
endif ()

# Print some helpful debug info
message(STATUS "Found kernel C compiler: ${MUOS_CC}")
message(STATUS "Found kernel C++ compiler: ${MUOS_CXX}")
message(STATUS "Found kernel NASM: ${MUOS_NASM}")
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
# These flags will be propagated to every single target built after calling find_package(MuToolchain).
# This guarantees that we use the same flags everywhere, for example: a library used by the kernel
# won't get built with red zone enabled (unless someone explicitly does stupid stuff).
set(_MU_CFLAGS_DEFAULT "-D__is_kernel_build__=1 -fstack-protector-all -ffreestanding -mcmodel=large -mno-red-zone -mno-sse -mno-avx -fno-tree-vectorize")
set(_MU_CXXFLAGS_DEFAULT "${_MU_CFLAGS_DEFAULT} -fno-rtti -fno-exceptions -fno-threadsafe-statics")
set(_MU_LINKFLAGS_DEFAULT "-nostdlib -z max-page-size=0x1000")

# Inject our default C/C++ flags into the compiler command line
set(CMAKE_C_COMPILE_OBJECT "<CMAKE_C_COMPILER> <DEFINES> <INCLUDES> ${_MU_CFLAGS_DEFAULT} <FLAGS> -o <OBJECT> -c <SOURCE>")
set(CMAKE_CXX_COMPILE_OBJECT "<CMAKE_CXX_COMPILER> <DEFINES> <INCLUDES> ${_MU_CXXFLAGS_DEFAULT} <FLAGS> -o <OBJECT> -c <SOURCE>")
set(_MU_OBJECT_LINK_ORDER "${_MU_KERNEL_CRTI_OBJ} ${_MU_CRTBEGIN_PATH} <OBJECTS> ${_MU_CRTEND_PATH} ${_MU_KERNEL_CRTN_OBJ} <LINK_LIBRARIES>")
set(CMAKE_CXX_LINK_EXECUTABLE "<CMAKE_CXX_COMPILER> <FLAGS> <CMAKE_CXX_LINK_FLAGS> ${_MU_LINKFLAGS_DEFAULT} <LINK_FLAGS> -Xlinker -x ${_MU_OBJECT_LINK_ORDER} -o <TARGET>")
# Use NASM for .asm and .nasm files
set(CMAKE_ASM_NASM_OBJECT_FORMAT elf64)
set(CMAKE_ASM_NASM_SOURCE_FILE_EXTENSIONS "asm;nasm")
