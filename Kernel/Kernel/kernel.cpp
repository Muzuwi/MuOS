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

#include <Arch/i386/CPU.hpp>
#include <Kernel/Interrupt/IRQSubscriber.hpp>
#include <Arch/i386/PortIO.hpp>
#include <Kernel/Memory/QuickMap.hpp>
#include <Kernel/Module.hpp>
#include <Kernel/ksleep.hpp>
#include <Kernel/Debug/kpanic.hpp>
#include <Kernel/Interrupt/Exception.hpp>
#include <Kernel/Device/PS2Keyboard.hpp>

#include <Kernel/Syscalls/SyscallList.hpp>
namespace uKernel {
	extern "C" void kernel_entrypoint(uintptr_t*);
};

/*
	Main kernel entrypoint
*/
extern "C" void uKernel::kernel_entrypoint(uintptr_t* multiboot_info){
#define PORT 0x3f8   /* COM1 */
	out(PORT + 1, 0x00);    // Disable all interrupts
	out(PORT + 3, 0x80);    // Enable DLAB (set baud rate divisor)
	out(PORT + 0, 0x01);    // Set divisor to 3 (lo byte) 38400 baud
	out(PORT + 1, 0x00);    //                  (hi byte)
	out(PORT + 3, 0x03);    // 8 bits, no parity, one stop bit
	out(PORT + 2, 0xC7);    // Enable FIFO, clear them, with 14-byte threshold

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

	PS2Keyboard::init();

	KMalloc::get().logAllocationStats();

	kdebugf("[uKernel] Initialization took %ims\n", (int)Timer::getTimer().getTimeSinceStart());

	extern unsigned char testing[];
	extern unsigned testing_len;

	Process::create_from_ELF(
			&testing[0],
			testing_len
	);

	new IRQSubscriber(12, []{
		ASSERT_IRQ_DISABLED();
		uint8_t data = in(0x60);
		kdebugf("Mouse data: %x\n", data);
	});

	Scheduler::enter_scheduler_loop();
}
