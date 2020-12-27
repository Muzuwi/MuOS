include(../Toolchain/CMakeLists.txt)

#
#  CRT objects, required by the compiler
#

add_library(CRTi OBJECT
    Arch/i386/crti.S
)

add_library(CRTn OBJECT
    Arch/i386/crtn.S
)

add_custom_command(
    OUTPUT crtbegin.o
    COMMAND ${CMAKE_COMMAND} -E copy `${CMAKE_C_COMPILER} -print-file-name=crtbegin.o` crtbegin.o
)

add_custom_command(
    OUTPUT crtend.o
    COMMAND ${CMAKE_COMMAND} -E copy `${CMAKE_C_COMPILER} -print-file-name=crtend.o` crtend.o
)

add_library(CRTbegin OBJECT
    crtbegin.o
)

add_library(CRTend OBJECT
    crtend.o
)


#
#   Kernel entrypoint
#   GRUB calls _entrypoint_x32
#   The 32-bit entrypoint provides a small single-page stack, initial GDT and null IDT
#   After setting up long mode, it jumps to _entrypoint_x64, while passing along the multiboot
#   info header. The callee is then responsible for setting up its' own stack and G/IDT
#
add_library(Entrypoint
    Bootstage/Entrypoint64.asm
    Bootstage/Boot32.asm
)

target_compile_options(Entrypoint
    PRIVATE $<$<OR:$<COMPILE_LANGUAGE:CXX>,$<COMPILE_LANGUAGE:C>>:-ffreestanding -mcmodel=large>
    PRIVATE $<$<OR:$<COMPILE_LANGUAGE:CXX>,$<COMPILE_LANGUAGE:C>>:-fstack-protector-all>
    PRIVATE $<$<COMPILE_LANGUAGE:CXX>:--std=c++2a>
    PRIVATE $<$<COMPILE_LANGUAGE:ASM_NASM>:-f elf64 -I ${CMAKE_SOURCE_DIRECTORY}\\>
)


#
#   Kernel data
#
add_library(Kernel
    Kernel/kernel.cpp
    Kernel/stackchk.cpp
    Debug/kdebugf.cpp
    Debug/kpanic.cpp
    Debug/kassert.cpp
    Debug/TTY.cpp
    Arch/i386/PortIO.cpp
    Arch/i386/VGA.cpp

    Arch/i386/GDT.cpp
    Arch/i386/asm/LGDT.asm

    Arch/i386/IDT.cpp
    Interrupt/asm/ExceptionEntrypoint.asm
    Interrupt/asm/IRQEntrypoint.asm

)

add_dependencies(Kernel
    Entrypoint
    CRTi
    CRTn
    CRTbegin
    CRTend
    LibC
    LibGeneric
)

target_compile_options(Kernel
    PRIVATE $<$<OR:$<COMPILE_LANGUAGE:CXX>,$<COMPILE_LANGUAGE:C>>:-ffreestanding -mcmodel=large>
    PRIVATE $<$<OR:$<COMPILE_LANGUAGE:CXX>,$<COMPILE_LANGUAGE:C>>:-fstack-protector-all>
    PRIVATE $<$<COMPILE_LANGUAGE:CXX>:--std=c++2a>
    PRIVATE $<$<COMPILE_LANGUAGE:ASM_NASM>:-f elf64>
)

target_compile_definitions(Kernel
    PRIVATE __is_kernel_build__
)


#add_library(Kernel
#        Arch/i386/asm/LGDT.asm
#        Arch/i386/GDT.cpp
#        Arch/i386/IDT.cpp
#        Arch/i386/kernel_link.ld
#        Arch/i386/Page.cpp
#        Arch/i386/PageDirectory.cpp
#        Arch/i386/PagingBootstrap.cpp
#        Arch/i386/tty.c
#        Arch/i386/vga.c
#        Bootstage/boot.asm
#        Bootstage/BootConfig.cpp
#        Debug/kassert.cpp
#        Debug/kdebugf.cpp
#        Debug/kpanic.cpp
#        Device/i8042.asm
#        Device/i8042.cpp
#        Device/IDE/IDE_Channel.cpp
#        Device/IDE/IDE_Drive.cpp
#        Device/IDE.cpp
#        Device/PCI.cpp
#        Device/Timer.cpp
#        Filesystem/VDM.cpp
#        include/Arch/i386/BootConfig.hpp
#        include/Arch/i386/GDT.hpp
#        include/Arch/i386/i8042.hpp
#        include/Arch/i386/IDT.hpp
#        include/Arch/i386/Multiboot.hpp
#        include/Arch/i386/Page.hpp
#        include/Arch/i386/PageDirectory.hpp
#        include/Arch/i386/PortIO.hpp
#        include/Arch/i386/Registers.hpp
#        include/Arch/i386/Timer.hpp
#        include/Arch/i386/vga.h
#        include/Kernel/Debug/kassert.hpp
#        include/Kernel/Debug/kdebugf.hpp
#        include/Kernel/Debug/kpanic.hpp
#        include/Kernel/Debug/tty.h
#        include/Kernel/Device/IDE/IDE_Channel.hpp
#        include/Kernel/Device/IDE/IDE_Drive.hpp
#        include/Kernel/Device/IDE.hpp
#        include/Kernel/Device/PCI.hpp
#        include/Kernel/Filesystem/GenericFS.hpp
#        include/Kernel/Filesystem/VDM.hpp
#        include/Kernel/Filesystem/VirtualBlockDevice.hpp
#        include/Kernel/ksleep.hpp
#        include/Kernel/Memory/kmalloc.hpp
#        include/Kernel/Memory/PMM.hpp
#        include/Kernel/Memory/VMM.hpp
#        include/Kernel/Process/Process.hpp
#        include/Kernel/SystemTypes.hpp
#        Interrupt/asm/ExceptionEntrypoint.asm
#        Interrupt/ExceptionHandler.cpp
#        Interrupt/SyscallHandler.cpp
#        Interrupt/asm/IRQEntrypoint.asm
#        Interrupt/IRQDispatcher.cpp
#        Interrupt/IRQSubscriber.cpp
#        Kernel/kernel.cpp
#        Kernel/ksleep.cpp
#        Memory/kmalloc.cpp
#        Memory/PMM.cpp
#        Memory/VMM.cpp
#        Kernel/stackchk.cpp
#        Arch/i386/IRQDisabler.hpp
#        Process/Process.cpp
#        Process/Scheduler.cpp
#        Arch/i386/CPU.cpp
#        Memory/QuickMap.cpp
#        Syscalls/Syscalls.cpp
#        Kernel/test.cpp
#        Process/ProcessFinalize.asm
#        Process/SwitchTask.asm
#        Arch/i386/asm/CPU_jump_to_irq_frame.asm
#        Process/ImplSyscall.cpp
#    Arch/i386/Paging/PDE.cpp Arch/i386/Paging/PDPTE.cpp Arch/i386/Paging/PML4E.cpp Arch/i386/Paging/PTE.cpp
#)