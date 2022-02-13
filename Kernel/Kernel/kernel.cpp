#include <Debug/TTY.hpp>
#include <Memory/KHeap.hpp>
#include <Memory/PMM.hpp>
#include <Multiboot/MultibootInfo.hpp>
#include <Arch/x86_64/GDT.hpp>
#include <Arch/x86_64/IDT.hpp>

#include <Device/PIT.hpp>
#include <Arch/x86_64/CPU.hpp>
#include <Process/Process.hpp>
#include <Syscalls/Syscall.hpp>

#include <APIC/APIC.hpp>
#include <ACPI/ACPI.hpp>

#include <Device/Serial.hpp>
#include <SMP/SMP.hpp>
#include <Debug/klogf.hpp>
#include <Debug/kpanic.hpp>

/*
	Main kernel entrypoint
*/
extern "C" [[noreturn]] void _ukernel_entrypoint(PhysPtr<MultibootInfo> multiboot_info) {
	TTY::init();
	Serial::init();
	klogf_static("[uKernel64] Hello, world!\n");

	IDT::init();
	GDT::init();
	CPU::initialize_features();

	SMP::bootstrap_ctb();
	CPU::irq_enable();

	PMM::instance().init_regions(multiboot_info);
	VMM::initialize_kernel_vm();
	PMM::instance().init_deferred_allocators();

	KHeap::instance().init();

	Syscall::init();

	klogf("[uKernel] Init done, time passed: {}ms\n", PIT::milliseconds());

	ACPI::parse_tables();
	APIC::discover();
	SMP::init_control_blocks();
	SMP::ctb().scheduler().bootstrap();

	kpanic();
}
