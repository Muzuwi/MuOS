#include <Kernel/Debug/TTY.hpp>
#include <Kernel/Memory/KHeap.hpp>
#include <Kernel/Memory/PMM.hpp>
#include <Kernel/Multiboot/MultibootInfo.hpp>
#include <Arch/x86_64/GDT.hpp>
#include <Arch/x86_64/IDT.hpp>

#include <Kernel/Device/PIT.hpp>
#include <Arch/x86_64/CPU.hpp>
#include <Kernel/Process/Process.hpp>
#include <Kernel/Syscalls/Syscall.hpp>

#include <Kernel/APIC/APIC.hpp>
#include <Kernel/ACPI/ACPI.hpp>

#include <Kernel/Device/Serial.hpp>
#include <Kernel/SMP/SMP.hpp>
#include <Debug/klogf.hpp>

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

	KHeap::init();

	Syscall::init();

	klogf("[uKernel] Init done, time passed: {}ms\n", PIT::milliseconds());

	ACPI::parse_tables();
	APIC::discover();
	SMP::init_control_blocks();
	SMP::ctb().scheduler().bootstrap();

	kpanic();
}
