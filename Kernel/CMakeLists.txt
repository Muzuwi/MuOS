include(../Toolchain/CMakeLists.txt)

#
#  CRT objects, required by the compiler
#
add_custom_target(CRTi
    COMMAND ${CMAKE_ASM-ATT_COMPILER} -o ${CMAKE_CURRENT_BINARY_DIR}/CRTi.o ${CMAKE_SOURCE_DIR}/Kernel/Arch/x86_64/crti.S
    BYPRODUCTS ${CMAKE_CURRENT_BINARY_DIR}/CRTi.o)
add_custom_target(CRTn
    COMMAND ${CMAKE_ASM-ATT_COMPILER} -o ${CMAKE_CURRENT_BINARY_DIR}/CRTn.o ${CMAKE_SOURCE_DIR}/Kernel/Arch/x86_64/crtn.S
    BYPRODUCTS ${CMAKE_CURRENT_BINARY_DIR}/CRTn.o)
add_custom_target(CRTbegin
    COMMAND ${CMAKE_COMMAND} -E copy `${CMAKE_C_COMPILER} -print-file-name=crtbegin.o` ${CMAKE_CURRENT_BINARY_DIR}/CRTbegin.o
    BYPRODUCTS ${CMAKE_CURRENT_BINARY_DIR}/CRTbegin.o)
add_custom_target(CRTend
    COMMAND ${CMAKE_COMMAND} -E copy `${CMAKE_C_COMPILER} -print-file-name=crtend.o` ${CMAKE_CURRENT_BINARY_DIR}/CRTend.o
    BYPRODUCTS ${CMAKE_CURRENT_BINARY_DIR}/CRTend.o)

#
#   Kernel entrypoint
#   GRUB calls _entrypoint_x32
#   The 32-bit entrypoint provides a small single-page stack, initial GDT and null IDT
#   After setting up long mode, it jumps to _entrypoint_x64, while passing along the multiboot
#   info header. The callee is then responsible for setting up its' own stack and G/IDT
#
add_library(Entrypoint OBJECT
    Bootstage/Entrypoint64.asm
    Bootstage/Boot32.asm)

target_compile_options(Entrypoint
    PRIVATE $<$<OR:$<COMPILE_LANGUAGE:CXX>,$<COMPILE_LANGUAGE:C>>:-ffreestanding -mcmodel=large>
    PRIVATE $<$<OR:$<COMPILE_LANGUAGE:CXX>,$<COMPILE_LANGUAGE:C>>:-fstack-protector-all>
    PRIVATE $<$<COMPILE_LANGUAGE:CXX>:--std=c++2a>
    PRIVATE $<$<COMPILE_LANGUAGE:ASM_NASM>:-f elf64 -I ${CMAKE_SOURCE_DIRECTORY}\\>)


#
#   Kernel ELF
#
set(CMAKE_CXX_LINK_EXECUTABLE
    "<CMAKE_CXX_COMPILER> <FLAGS> <CMAKE_CXX_LINK_FLAGS> ${MU_LINKER_FLAGS} <LINK_FLAGS> -Xlinker -x ${CMAKE_CURRENT_BINARY_DIR}/CRTi.o ${CMAKE_CURRENT_BINARY_DIR}/CRTbegin.o <OBJECTS> <LINK_LIBRARIES> ${CMAKE_CURRENT_BINARY_DIR}/CRTend.o ${CMAKE_CURRENT_BINARY_DIR}/CRTn.o -o <TARGET>")

add_executable(KernelELF
    Kernel/kernel.cpp
    Kernel/stackchk.cpp
    Debug/kpanic.cpp
    Debug/kassert.cpp
    Debug/TTY.cpp
    Arch/x86_64/PortIO.cpp
    Arch/x86_64/VGA.cpp

    Arch/x86_64/GDT.cpp
    Arch/x86_64/asm/LGDT.asm

    Arch/x86_64/IDT.cpp
    Exception/asm/ExceptionEntrypoint.asm
    Interrupt/asm/IRQEntrypoint.asm

    Memory/PMM.cpp
    Memory/VMM.cpp
    Arch/x86_64/Paging/PTE.cpp
    Arch/x86_64/Paging/PDE.cpp
    Arch/x86_64/Paging/PDPTE.cpp
    Arch/x86_64/Paging/PML4E.cpp

    Arch/x86_64/CPUID.cpp

    Memory/KHeap.cpp
    Memory/Allocators/SlabAllocator.cpp

    Exception/ExceptionEntrypoint.cpp

    Interrupt/IRQDispatcher.cpp

    Device/PIT.cpp
    Kernel/ksleep.cpp

    Arch/x86_64/CPU.cpp
    Process/Process.cpp
    Process/PidAllocator.cpp
    Arch/x86_64/asm/SwitchTo.asm
    Scheduler/Scheduler.cpp
    Scheduler/RunQueue.cpp

    Syscalls/SysEntry.asm
    Syscalls/Syscall.cpp
    Locks/KMutex.cpp
    Process/ProcSyscalls.cpp
    Memory/Wrappers/UserPtr/UserPtrGeneric.cpp
    Memory/Wrappers/UserPtr/UserString.cpp

    ACPI/ACPI.cpp
    APIC/APIC.cpp
    Device/Serial.cpp
    SMP/SMP.cpp
    Process/ProcCreate.cpp
    Process/Thread.cpp
    Process/ThreadCreate.cpp
    Exception/Handlers.cpp

    Arch/x86_64/asm/V86.asm
    Arch/x86_64/V86.cpp
    SMP/APBoot.asm
    Structs/KAtomic.cpp
    Locks/KSemaphore.cpp
    Daemons/SysDbg/SysDbg.cpp
    Daemons/Kbd/Kbd.cpp
    Daemons/BootAP/BootAP.cpp
    Daemons/Idle/Idle.cpp
    Daemons/Testd/Testd.cpp
    Debug/DebugCon.cpp
    Memory/Wrappers/VMapping.cpp Memory/Allocators/PageBitmapAllocator.cpp Memory/Allocators/VBitmap.cpp Memory/Allocators/ChunkAllocator.hpp Memory/Allocators/ChunkAllocator.cpp Memory/Allocators/BumpAllocator.hpp Memory/Allocators/BumpAllocator.cpp SMP/MigrationService.cpp SMP/MigrationService.hpp)

target_compile_options(KernelELF
    PRIVATE $<$<OR:$<COMPILE_LANGUAGE:CXX>,$<COMPILE_LANGUAGE:C>>:-O2 -g -Wall -Wextra -ffreestanding -mcmodel=large -Werror=unused-result>
    PRIVATE $<$<OR:$<COMPILE_LANGUAGE:CXX>,$<COMPILE_LANGUAGE:C>>:-fstack-protector-all>
    PRIVATE $<$<OR:$<COMPILE_LANGUAGE:CXX>,$<COMPILE_LANGUAGE:C>>:-mno-red-zone>
    PRIVATE $<$<COMPILE_LANGUAGE:CXX>:--std=c++2a>
    PRIVATE $<$<COMPILE_LANGUAGE:ASM_NASM>:-f elf64>)

target_compile_definitions(KernelELF
    PRIVATE __is_kernel_build__)

add_dependencies(KernelELF
    CRTi
    CRTn
    CRTbegin
    CRTend)

target_link_libraries(KernelELF
    Entrypoint
    LibGeneric
    LibC)

add_custom_command(TARGET KernelELF
    POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_SOURCE_DIR}/Root/
    COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:KernelELF> ${CMAKE_SOURCE_DIR}/Root/uKernel.bin
    COMMENT "Copying kernel executable to Root directory ${CMAKE_SOURCE_DIR}/Root ")
