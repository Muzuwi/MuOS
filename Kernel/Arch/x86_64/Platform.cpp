#include <Arch/Interface.hpp>
#include <Arch/x86_64/Boot/MultibootInfo.hpp>
#include <Arch/x86_64/CPU.hpp>
#include <Arch/x86_64/GDT.hpp>
#include <Arch/x86_64/IDT.hpp>
#include <Arch/x86_64/Interrupt.hpp>
#include <Arch/x86_64/MP/Boot.hpp>
#include <Arch/x86_64/MP/ExecutionEnvironment.hpp>
#include <Arch/x86_64/PCI/PCI.hpp>
#include <Arch/x86_64/Serial.hpp>
#include <Arch/x86_64/VGAConsole.hpp>
#include <Core/Error/Error.hpp>
#include <Core/MP/MP.hpp>
#include <Core/Start/Start.hpp>
#include <Memory/PMM.hpp>
#include <Memory/VMM.hpp>
#include <Syscalls/Syscall.hpp>
#include <SystemTypes.hpp>
#include "ACPI.hpp"
#include "APIC.hpp"
#include "Arch/x86_64/SerialConsole.hpp"

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
	//  Initialize the execution environment
	//  This must be done as soon as possible
	void* env = arch::mp::create_environment();
	CPU::set_gs_base(env);
	CPU::set_kernel_gs_base(env);
	this_execution_environment()->gdt.load();
	IDT::init();
	//  Force reload GSBASE after changing GDT
	CPU::set_gs_base(env);
	CPU::set_kernel_gs_base(env);

	(void)vgacon::init();
	Serial::init();
	(void)serialcon::init();
	CPU::initialize_features();
	PMM::instance().init_regions(s_multiboot_context);

	return core::Error::Ok;
}

core::Error arch::platform_init() {
	irq_local_enable();

	PCI::discover();
	Syscall::init();
	ACPI::parse_tables();
	APIC::discover();
	arch::mp::boot_aps();

	return core::Error::Ok;
}

void arch::mp::environment_set(void* env) {
	this_execution_environment()->environment = static_cast<core::mp::Environment*>(env);
}

void* arch::mp::environment_get() {
	return this_execution_environment()->environment;
}
