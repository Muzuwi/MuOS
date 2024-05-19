# This file serves as a source for common build flags
# to use during compilation of the kernel and arch-specific
# modules.
# Mainly, this handles debug/release compilation modes and some
# common warning/error options.

set(MUOS_CXX_FLAGS
    -Wall
    -Wextra
    -Werror=unused-result
    -Werror=return-type
    --std=c++2a
    $<$<CONFIG:Debug>:-ggdb -g>
    $<$<CONFIG:Release>:-O2>
    $<$<CONFIG:RelWithDebInfo>:-O2 -ggdb -g>
    )

# FIXME: Remove once remaining ASM from the main kernel is moved
# to a machine-specific directory.
# This should only be ever needed from within arch-specific CMakeLists.
set(MUOS_ASM_FLAGS
    $<$<STREQUAL:${MU_MACHINE},x86_64>:-f elf64>
    )
