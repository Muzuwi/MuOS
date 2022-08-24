#include <Arch/x86_64/ACPI.hpp>
#include <Arch/x86_64/APIC.hpp>
#include <Arch/x86_64/CPU.hpp>
#include <Arch/x86_64/GDT.hpp>
#include <Arch/x86_64/IDT.hpp>
#include <Arch/x86_64/PIT.hpp>
#include <Bootstage/MultibootInfo.hpp>
#include <Debug/klogf.hpp>
#include <Debug/kpanic.hpp>
#include <Debug/TTY.hpp>
#include <Device/Serial.hpp>
#include <Memory/KHeap.hpp>
#include <Memory/PMM.hpp>
#include <Process/Process.hpp>
#include <SMP/SMP.hpp>
#include <Syscalls/Syscall.hpp>

/*
    Main kernel entrypoint
*/
extern "C" [[noreturn]] void _ukernel_entrypoint(PhysPtr<MultibootInfo> multiboot_info) {
	TTY::init();
	Serial::init();
	klogf_static("[uKernel64] Hello, world!\n");

	IDT::init();
	GDT::init_base_ap_gdt();
	CPU::initialize_features();

	SMP::reload_boot_ctb();
	CPU::irq_enable();

	PMM::instance().init_regions(multiboot_info);
	VMM::initialize_kernel_vm();
	PMM::instance().init_deferred_allocators();

	KHeap::instance().init();

	Syscall::init();

	klogf("[uKernel] Init done, time passed: {}ms\n", PIT::milliseconds());

	ACPI::parse_tables();
	APIC::discover();
	SMP::attach_boot_ap();
	this_cpu().scheduler().bootstrap();

	kpanic();
}
