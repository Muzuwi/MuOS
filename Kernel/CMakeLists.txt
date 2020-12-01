include(../Toolchain/CMakeLists.txt)

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
    crtbegin.o)

add_library(CRTend OBJECT
    crtend.o)

add_library(Kernel
        Arch/i386/asm/LGDT.asm
        Arch/i386/GDT.cpp
        Arch/i386/IDT.cpp
        Arch/i386/kernel_link.ld
        Arch/i386/Page.cpp
        Arch/i386/PageDirectory.cpp
        Arch/i386/PagingBootstrap.cpp
        Arch/i386/PortIO.cpp
        Arch/i386/tty.c
        Arch/i386/vga.c
        Bootstage/boot.asm
        Bootstage/BootConfig.cpp
        Debug/kassert.cpp
        Debug/kdebugf.cpp
        Debug/kpanic.cpp
        Device/i8042.asm
        Device/i8042.cpp
        Device/IDE/IDE_Channel.cpp
        Device/IDE/IDE_Drive.cpp
        Device/IDE.cpp
        Device/PCI.cpp
        Device/Timer.cpp
        Filesystem/VDM.cpp
        include/Arch/i386/BootConfig.hpp
        include/Arch/i386/GDT.hpp
        include/Arch/i386/i8042.hpp
        include/Arch/i386/IDT.hpp
        include/Arch/i386/Multiboot.hpp
        include/Arch/i386/Page.hpp
        include/Arch/i386/PageDirectory.hpp
        include/Arch/i386/PortIO.hpp
        include/Arch/i386/Registers.hpp
        include/Arch/i386/Timer.hpp
        include/Arch/i386/vga.h
        include/Kernel/Debug/kassert.hpp
        include/Kernel/Debug/kdebugf.hpp
        include/Kernel/Debug/kpanic.hpp
        include/Kernel/Debug/tty.h
        include/Kernel/Device/IDE/IDE_Channel.hpp
        include/Kernel/Device/IDE/IDE_Drive.hpp
        include/Kernel/Device/IDE.hpp
        include/Kernel/Device/PCI.hpp
        include/Kernel/Filesystem/GenericFS.hpp
        include/Kernel/Filesystem/VDM.hpp
        include/Kernel/Filesystem/VirtualBlockDevice.hpp
        include/Kernel/ksleep.hpp
        include/Kernel/Memory/kmalloc.hpp
        include/Kernel/Memory/PMM.hpp
        include/Kernel/Memory/VMM.hpp
        include/Kernel/Process/Process.hpp
        include/Kernel/SystemTypes.hpp
        Interrupt/asm/ExceptionEntrypoint.asm
        Interrupt/ExceptionHandler.cpp
        Interrupt/SyscallHandler.cpp
        Interrupt/asm/IRQEntrypoint.asm
        Interrupt/IRQDispatcher.cpp
        Interrupt/IRQSubscriber.cpp
        Kernel/kernel.cpp
        Kernel/ksleep.cpp
        Memory/kmalloc.cpp
        Memory/PMM.cpp
        Memory/VMM.cpp
        Kernel/stackchk.cpp
        Arch/i386/IRQDisabler.hpp
        Process/Process.cpp
        Process/Scheduler.cpp
        Arch/i386/CPU.cpp
        Memory/QuickMap.cpp
        Syscalls/Syscalls.cpp
        Kernel/test.cpp
        Process/ProcessFinalize.asm
        Process/SwitchTask.asm
        Arch/i386/asm/CPU_jump_to_irq_frame.asm
        Process/ImplSyscall.cpp
)

add_dependencies(Kernel
        CRTi
        CRTn
        CRTbegin
        CRTend
        LibC
        LibGeneric
)

target_compile_options(Kernel
        PRIVATE $<$<OR:$<COMPILE_LANGUAGE:CXX>,$<COMPILE_LANGUAGE:C>>:-ffreestanding>
        PRIVATE $<$<OR:$<COMPILE_LANGUAGE:CXX>,$<COMPILE_LANGUAGE:C>>:-fstack-protector-all>
        PRIVATE $<$<COMPILE_LANGUAGE:CXX>:--std=c++2a>
)

target_compile_definitions(Kernel
        PRIVATE __is_kernel_build__
)