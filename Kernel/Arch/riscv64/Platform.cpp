#include <Arch/Platform.hpp>
#include <Arch/riscv64/SBI.hpp>
#include <Arch/riscv64/SbiConsole.hpp>
#include <Core/Assert/Assert.hpp>
#include <Core/Log/Logger.hpp>
#include <SystemTypes.hpp>

CREATE_LOGGER("rv64::boot", core::log::LogLevel::Debug);

static void platform_rv64_print_sbi_info() {
	//  Print data regarding the SBI implementation the machine is using.
	//  Doesn't serve any particular purpose, but should help with debugging.
	const auto spec_ver =
	        ::arch::rv64::sbi_call(0, 0, 0, 0, 0, 0, arch::rv64::SBI_FID_GET_SPEC_VERSION, arch::rv64::SBI_EID_BASE);
	if(spec_ver.ok()) {
		::log.info("SBI specification version: {x}", spec_ver.value);
	}
	const auto impl_id =
	        ::arch::rv64::sbi_call(0, 0, 0, 0, 0, 0, arch::rv64::SBI_FID_GET_IMPL_ID, arch::rv64::SBI_EID_BASE);
	if(impl_id.ok()) {
		::log.info("SBI implementation ID: {x}", impl_id.value);
	}
	const auto impl_ver =
	        ::arch::rv64::sbi_call(0, 0, 0, 0, 0, 0, arch::rv64::SBI_FID_GET_IMPL_VER, arch::rv64::SBI_EID_BASE);
	if(impl_ver.ok()) {
		::log.info("SBI implementation version: {x}", impl_ver.value);
	}

	//  Machine information calls
	const auto mach_vnd_id =
	        ::arch::rv64::sbi_call(0, 0, 0, 0, 0, 0, arch::rv64::SBI_FID_GET_MACH_VENDOR_ID, arch::rv64::SBI_EID_BASE);
	if(mach_vnd_id.ok()) {
		::log.info("Machine vendor: {x}", mach_vnd_id.value);
	}
	const auto mach_arch_id =
	        ::arch::rv64::sbi_call(0, 0, 0, 0, 0, 0, arch::rv64::SBI_FID_GET_MACH_ARCH_ID, arch::rv64::SBI_EID_BASE);
	if(mach_arch_id.ok()) {
		::log.info("Machine architecture: {x}", mach_arch_id.value);
	}
	const auto mach_impl_id =
	        ::arch::rv64::sbi_call(0, 0, 0, 0, 0, 0, arch::rv64::SBI_FID_GET_MACH_IMPL_ID, arch::rv64::SBI_EID_BASE);
	if(mach_impl_id.ok()) {
		::log.info("Machine implementation ID: {x}", mach_impl_id.value);
	}
}

extern "C" {
	[[maybe_unused]] void platform_trap_handler(uint64 scause, uint64 sepc, uint64 stval) {
		log.error("Caught trap!");
		log.error("SCAUSE={x} SEPC={x} STVAL={x}", scause, sepc, stval);
		core::panic("Encountered an unhandled trap!");
	}

	[[maybe_unused]] [[noreturn]] void platform_boot_entry(void* /*fdt*/) {
		if(const auto err = arch::rv64::init_sbi_earlycon(); err != core::Error::Ok) {
			//  This won't get printed, however a panic is the only thing we can do.
			core::panic("Failed to initialize OpenSBI early console");
		}
		::log.info("SBI console initialized");
		platform_rv64_print_sbi_info();

		//  For testing, simply do nothing in the actual kernel for now
		HANG();
	}
}