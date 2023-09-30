#include <Arch/Interface.hpp>
#include <Arch/x86_64/Boot/MultibootInfo.hpp>
#include <Arch/x86_64/CPU.hpp>
#include <Arch/x86_64/GDT.hpp>
#include <Arch/x86_64/IDT.hpp>
#include <Arch/x86_64/PCI/PCI.hpp>
#include <Arch/x86_64/Serial.hpp>
#include <Core/Start/Start.hpp>
#include <Debug/klogf.hpp>
#include <Debug/TTY.hpp>
#include <Memory/PMM.hpp>
#include <Memory/VMM.hpp>
#include <SMP/SMP.hpp>
#include <Syscalls/Syscall.hpp>
#include "ACPI.hpp"
#include "APIC.hpp"

static PhysPtr<MultibootInfo> s_multiboot_context;

extern "C" [[noreturn, maybe_unused]] void platform_boot_entry(void* context) {
	//  Save the multiboot context
	gen::construct_at(&s_multiboot_context, PhysPtr<MultibootInfo>(reinterpret_cast<MultibootInfo*>(context)));
	//  Jump to the actual kernel boot
	core::start::start();
	//  Should never be reached
	while(true)
		;
}

core::Error arch::platform_early_init() {
	TTY::init();
	Serial::init();
	klogf_static("[uKernel64] Hello, world!\n");

	IDT::init();
	GDT::init_base_ap_gdt();
	CPU::initialize_features();
	SMP::reload_boot_ctb();
	CPU::irq_enable();

	PMM::instance().init_regions(s_multiboot_context);
	VMM::initialize_kernel_vm();
	PMM::instance().init_deferred_allocators();

	return core::Error::Ok;
}

core::Error arch::platform_init() {
	PCI::discover();
	Syscall::init();
	ACPI::parse_tables();
	APIC::discover();
	SMP::attach_boot_ap();

	return core::Error::Ok;
}
