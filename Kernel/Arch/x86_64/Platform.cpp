#include <Arch/MP.hpp>
#include <Arch/Platform.hpp>
#include <Arch/x86_64/ACPI.hpp>
#include <Arch/x86_64/APIC.hpp>
#include <Arch/x86_64/CPU.hpp>
#include <Arch/x86_64/Interrupt.hpp>
#include <Arch/x86_64/MP/Boot.hpp>
#include <Arch/x86_64/MP/ExecutionEnvironment.hpp>
#include <Arch/x86_64/PCI/PCI.hpp>
#include <Arch/x86_64/Serial.hpp>
#include <Arch/x86_64/SerialConsole.hpp>
#include <Arch/x86_64/VGAConsole.hpp>
#include <Core/Error/Error.hpp>
#include <Core/Mem/VM.hpp>
#include <Core/MP/MP.hpp>
#include <Memory/VMM.hpp>
#include <Syscalls/Syscall.hpp>
#include <SystemTypes.hpp>

CREATE_LOGGER("boot::x86_64", core::log::LogLevel::Debug);

core::Error arch::platform_early_init() {
	(void)vgacon::init();
	Serial::init();
	(void)serialcon::init();
	CPU::initialize_features();

	//  Load CR3 with the proper kernel mappings
	//  Prior to doing this, the kernel is running with the bootstrap mappings
	//  provided by the preloader.
	asm volatile("mov %%rax, %0\n"
	             "mov %%cr3, %%rax\n"
	             :
	             : "r"(core::mem::get_vmroot()));

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
	//  Only need to set the *current* GS base
	//  In kernel mode, the "kernel GS base" contains the userspace GS base.
	CPU::set_gs_base(env);
}

void* arch::mp::environment_get() {
	//  Read the GS base via a dedicated self-reference pointer found in the environment.
	//  This requires integration with the core::mp::Environment struct.
	auto read_env = []() -> void* {
		uint64 data;
		asm volatile("mov %0, %%gs:0\n" : "=r"(data) :);
		return (void*)data;
	};
	return static_cast<void*>(read_env());
}
