#include <stdio.h>
#include <string.h>
#include <Kernel/Debug/tty.h>
#include <Arch/i386/i8042.hpp>
#include <Arch/i386/gdt.hpp>
#include <Arch/i386/interrupts.hpp>
#include <Kernel/Debug/kdebugf.hpp>
#include <Arch/i386/timer.hpp>
#include <Arch/i386/paging.hpp>
#include <Arch/i386/BootConfig.hpp>
#include <Arch/i386/MemManager.hpp>
#include <Kernel/Memory/kmalloc.hpp>
#include <Kernel/Device/PCI.hpp>
#include <Kernel/Device/IDE.hpp>
#include <Kernel/Filesystem/VDM.hpp>

namespace uKernel {
	extern "C" void kernel_entrypoint(uintptr_t*);
};

/*
	Main kernel entrypoint
*/
extern "C" void uKernel::kernel_entrypoint(uintptr_t* multiboot_info){
	tty_init();
	kdebugf("[uKernel] uKernel booting\n");

	KMalloc::get().init();
	
	GDT::init_GDT();
	IDT::init_PIC();
	IDT::init_IDT();

	BootConfig::parse_multiboot_structure(multiboot_info);

	//  Find pci devices connected to the system
	PCI::discover_devices();

	kdebugf("[uKernel] Found %i PCI devices\n", PCI::getDevices().size());

	IDE::check_devices();

	// i8042::init_controller();

	VDM::debug();

	KMalloc::get().logAllocationStats();

	kdebugf("[uKernel] Initialization took %ims\n", (int)Timer::getTimer().getTimeSinceStart());
	while(true){
		// kdebugf("%ims\n", (int)Timer::getTimer().getTimeSinceStart());
	}
}
