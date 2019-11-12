#include <stdio.h>
#include <string.h>
#include <kernel/tty.h>
#include <arch/i386/i8042.hpp>
#include <arch/i386/gdt.hpp>
#include <arch/i386/interrupts.hpp>
#include <kernel/kdebugf.hpp>
#include <arch/i386/timer.hpp>
#include <arch/i386/paging.hpp>
#include <arch/i386/BootConfig.hpp>
#include <arch/i386/MemManager.hpp>

namespace uKernel {
	extern "C" void kernel_entrypoint(uintptr_t*);
};

/*
	Main kernel entrypoint
*/
extern "C" void uKernel::kernel_entrypoint(uintptr_t* multiboot_info){
	tty_init();
	kdebugf("[uKernel] uKernel booting\n");

	Paging::init_paging();
	BootConfig::parse_multiboot_structure(multiboot_info);

	GDT::init_GDT();
	IDT::init_PIC();
	IDT::init_IDT();
	// i8042::init_controller();

	kdebugf("[uKernel] Initialization took %ims\n", (int)Timer::getTimer().getTimeSinceStart());
	while(true){
		// kdebugf("%ims\n", (int)Timer::getTimer().getTimeSinceStart());
	}
}