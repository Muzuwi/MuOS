#include <Kernel/Debug/TTY.hpp>
#include <Kernel/Debug/kdebugf.hpp>
#include <Kernel/Memory/KHeap.hpp>
#include <Kernel/Memory/PMM.hpp>
#include <Kernel/Multiboot/MultibootInfo.hpp>
#include <Arch/i386/GDT.hpp>
#include <Arch/i386/IDT.hpp>

#include <Kernel/Device/PIT.hpp>
#include <Arch/i386/CPU.hpp>
#include <Kernel/Process/Process.hpp>
#include <Kernel/Syscalls/Syscall.hpp>

#include <Kernel/APIC/APIC.hpp>
#include <Kernel/ACPI/ACPI.hpp>

#include <Kernel/Device/Serial.hpp>
#include <Kernel/SMP/SMP.hpp>

/*
	Main kernel entrypoint
*/
extern "C" [[noreturn]] void _ukernel_entrypoint(PhysPtr<MultibootInfo> multiboot_info){
	TTY::init();
	Serial::init();
	kdebugf("[uKernel64] Hello, world!\n");

	IDT::init();
	GDT::init();
	CPU::initialize_features();

	SMP::bootstrap_ctb();
	CPU::irq_enable();

	PMM::handle_multiboot_memmap(multiboot_info);
	VMM::initialize_kernel_vm();
	PMM::initialize_deferred_regions();

	KHeap::init();

	Syscall::init();

	kdebugf("[uKernel] Init done, time passed: %ims\n", PIT::milliseconds());

	ACPI::parse_tables();
	APIC::discover();
	SMP::init_control_blocks();
	SMP::ctb().scheduler().bootstrap();

	kpanic();
}
