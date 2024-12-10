#include <Arch/Platform.hpp>
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
		//  For testing, simply do nothing in the actual kernel for now
		HANG();
	}
}