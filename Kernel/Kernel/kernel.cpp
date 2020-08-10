#include <stdio.h>
#include <string.h>
#include <Kernel/Debug/tty.h>
#include <Arch/i386/i8042.hpp>
#include <Arch/i386/GDT.hpp>
#include <Arch/i386/IDT.hpp>
#include <Kernel/Debug/kdebugf.hpp>
#include <Arch/i386/Timer.hpp>
#include <Arch/i386/BootConfig.hpp>
#include <Kernel/Memory/kmalloc.hpp>
#include <Kernel/Device/PCI.hpp>
#include <Kernel/Device/IDE.hpp>
#include <Kernel/Filesystem/VDM.hpp>
#include <Kernel/Memory/VMM.hpp>
#include <Kernel/Process/Process.hpp>
#include <Kernel/Process/Scheduler.hpp>

#include <Kernel/Syscalls/SyscallList.hpp>
namespace uKernel {
	extern "C" void kernel_entrypoint(uintptr_t*);
};

/*
	Main kernel entrypoint
*/
extern "C" void uKernel::kernel_entrypoint(uintptr_t* multiboot_info){
	tty_init();
	Timer::getTimer();

	kdebugf("[uKernel] uKernel booting\n");


	GDT::init_GDT();
	IDT::init_PIC();
	IDT::init_IDT();

	VMM::init();
	CPU::initialize_features();
	BootConfig::parse_multiboot_structure(multiboot_info);

	//  Find pci devices connected to the system
	PCI::discover_devices();

	kdebugf("[uKernel] Found %i PCI devices\n", PCI::getDevices().size());

	IDE::check_devices();

    i8042::init_controller();

	VDM::debug();

	Scheduler::initialize();
	Syscall::init();

	KMalloc::get().logAllocationStats();

	kdebugf("[uKernel] Initialization took %ims\n", (int)Timer::getTimer().getTimeSinceStart());

	Scheduler::enter_scheduler_loop();
}
