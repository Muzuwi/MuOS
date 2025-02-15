#include <Arch/Platform.hpp>
#include <Arch/riscv64/SbiConsole.hpp>
#include <Core/Assert/Assert.hpp>
#include <Core/Log/Logger.hpp>
#include <SystemTypes.hpp>

CREATE_LOGGER("rv64::boot", core::log::LogLevel::Debug);

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

		//  For testing, simply do nothing in the actual kernel for now
		HANG();
	}
}