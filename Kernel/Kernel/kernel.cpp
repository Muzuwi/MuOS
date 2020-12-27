#include <stdint.h>
#include <Kernel/Debug/TTY.hpp>
#include <Kernel/Debug/kdebugf.hpp>

/*
	Main kernel entrypoint
*/
extern "C" void _ukernel_entrypoint(void* multiboot_info){ //  FIXME:  Nasty naked void*
	TTY::init();
	kdebugf("[uKernel64] Hello, world!\n");
	kdebugf("Multiboot ptr %x%x\n", ((uint64_t)multiboot_info>>32u), (uint64_t)multiboot_info&0xffffffffu);

//	kdebugf("[uKernel] uKernel booting\n");
//
//
//	GDT::init_GDT();
//	IDT::init_PIC();
//	IDT::init_IDT();
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
