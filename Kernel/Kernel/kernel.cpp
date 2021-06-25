#include <Kernel/Debug/TTY.hpp>
#include <Kernel/Debug/kdebugf.hpp>
#include <Kernel/Memory/KHeap.hpp>
#include <Kernel/Memory/PMM.hpp>
#include <Kernel/Memory/VMM.hpp>
#include <Kernel/Multiboot/MultibootInfo.hpp>
#include <Arch/i386/GDT.hpp>
#include <Arch/i386/IDT.hpp>

#include <Arch/i386/PIT.hpp>
#include <Arch/i386/CPU.hpp>
#include <Kernel/Process/Process.hpp>
#include <Kernel/Scheduler/Scheduler.hpp>
#include <Kernel/Syscalls/Syscall.hpp>

#include <Kernel/APIC/APIC.hpp>
#include <Kernel/ACPI/ACPI.hpp>

#include <Kernel/Device/Serial.hpp>

/*
	Main kernel entrypoint
*/
extern "C" void _ukernel_entrypoint(PhysPtr<MultibootInfo> multiboot_info){
	TTY::init();
	Serial::init();
	kdebugf("[uKernel64] Hello, world!\n");

	IDT::init();
	GDT::init();
	CPU::initialize_features();

	CPU::irq_enable();

	PMM::handle_multiboot_memmap(multiboot_info);
	VMM::init();
	PMM::initialize_deferred_regions();

	KHeap::init();

	Syscall::init();

	kdebugf("[uKernel] Init done, time passed: %ims\n", PIT::milliseconds());

	ACPI::parse_tables();
	APIC::discover();

	Scheduler::init();

	while (true)
		asm volatile("hlt\n");
}
