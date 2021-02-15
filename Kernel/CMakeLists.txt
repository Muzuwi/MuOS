include(../Toolchain/CMakeLists.txt)

#
#  CRT objects, required by the compiler
#
add_custom_target(CRTi
                  COMMAND ${CMAKE_ASM-ATT_COMPILER} -o ${CMAKE_CURRENT_BINARY_DIR}/CRTi.o ${CMAKE_SOURCE_DIR}/Kernel/Arch/i386/crti.S
                  BYPRODUCTS ${CMAKE_CURRENT_BINARY_DIR}/CRTi.o)
add_custom_target(CRTn
                  COMMAND ${CMAKE_ASM-ATT_COMPILER} -o ${CMAKE_CURRENT_BINARY_DIR}/CRTn.o ${CMAKE_SOURCE_DIR}/Kernel/Arch/i386/crtn.S
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
               Debug/kdebugf.cpp
               Debug/kpanic.cpp
               Debug/kassert.cpp
               Debug/TTY.cpp
               Arch/i386/PortIO.cpp
               Arch/i386/VGA.cpp

               Arch/i386/GDT.cpp
               Arch/i386/asm/LGDT.asm

               Arch/i386/IDT.cpp
               Exception/asm/ExceptionEntrypoint.asm
               Interrupt/asm/IRQEntrypoint.asm

               Memory/kmalloc.cpp

               Memory/PMM.cpp
               Memory/VMM.cpp
               Arch/i386/Paging/PTE.cpp
               Arch/i386/Paging/PDE.cpp
               Arch/i386/Paging/PDPTE.cpp
               Arch/i386/Paging/PML4E.cpp

               Arch/i386/CPUID.cpp

               Memory/KHeap.cpp
               Memory/Allocators/SlabAllocator.cpp

               Exception/ExceptionEntrypoint.cpp

               Interrupt/IRQDispatcher.cpp
               )

target_compile_options(KernelELF
                       PRIVATE $<$<OR:$<COMPILE_LANGUAGE:CXX>,$<COMPILE_LANGUAGE:C>>:-ffreestanding -mcmodel=large>
                       PRIVATE $<$<OR:$<COMPILE_LANGUAGE:CXX>,$<COMPILE_LANGUAGE:C>>:-fstack-protector-all>
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