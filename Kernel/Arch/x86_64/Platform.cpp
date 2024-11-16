#include <Arch/MP.hpp>
#include <Arch/Platform.hpp>
#include <Arch/x86_64/ACPI.hpp>
#include <Arch/x86_64/APIC.hpp>
#include <Arch/x86_64/CPU.hpp>
#include <Arch/x86_64/Interrupt.hpp>
#include <Arch/x86_64/MP/ExecutionEnvironment.hpp>
#include <Arch/x86_64/PCI/PCI.hpp>
#include <Arch/x86_64/Serial.hpp>
#include <Arch/x86_64/SerialConsole.hpp>
#include <Arch/x86_64/VGAConsole.hpp>
#include <Core/Error/Error.hpp>
#include <Core/MP/MP.hpp>
#include <SystemTypes.hpp>

CREATE_LOGGER("boot::x86_64", core::log::LogLevel::Debug);

core::Error arch::platform_early_init() {
	(void)vgacon::init();
	Serial::init();
	(void)serialcon::init();
	CPU::initialize_features();

	return core::Error::Ok;
}

core::Error arch::platform_init() {
	irq_local_enable();

	PCI::discover();
	ACPI::parse_tables();
	APIC::discover();
	//  arch::mp::boot_aps();

	return core::Error::Ok;
}

void arch::mp::environment_set(void* env) {
	this_execution_environment()->environment = static_cast<core::mp::Environment*>(env);
}

void* arch::mp::environment_get() {
	return this_execution_environment()->environment;
}
