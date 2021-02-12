#include <Kernel/Debug/TTY.hpp>
#include <Kernel/Debug/kdebugf.hpp>
#include <Kernel/Memory/PMM.hpp>
#include <Kernel/Memory/VMM.hpp>
#include <Kernel/Multiboot/MultibootInfo.hpp>
#include <Arch/i386/GDT.hpp>
#include <Arch/i386/IDT.hpp>

/*
	Main kernel entrypoint
*/
extern "C" void _ukernel_entrypoint(PhysPtr<MultibootInfo> multiboot_info){
	TTY::init();
	kdebugf("[uKernel64] Hello, world!\n");

	IDT::init();
	GDT::init();
	PMM::handle_multiboot_memmap(multiboot_info);
	VMM::init();
	PMM::initialize_deferred_regions();

	while (true)
		asm volatile("cli\nhlt\n");
//
//	VMM::init();
//	CPU::initialize_features();
//	BootConfig::parse_multiboot_structure(multiboot_info);
//
//	//  Find pci devices connected to the system
//	PCI::discover_devices();
//
//	kdebugf("[uKernel] Found %i PCI devices\n", PCI::getDevices().size());
//
//	IDE::check_devices();
//
//    i8042::init_controller();
//
//	VDM::debug();
//
//	Scheduler::initialize();
//	Syscall::init();
//
//	KMalloc::get().logAllocationStats();
//
//	kdebugf("[uKernel] Initialization took %ims\n", (int)Timer::getTimer().getTimeSinceStart());
//
//	extern unsigned char testing[];
//	extern unsigned testing_len;
//
//	Process::create_from_ELF(
//			&testing[0],
//			testing_len
//	);
//
//	new IRQSubscriber(12, []{
//		ASSERT_IRQ_DISABLED();
//		uint8_t data = in(0x60);
//		kdebugf("Mouse data: %x\n", data);
//	});
//
//	Scheduler::enter_scheduler_loop();
}
