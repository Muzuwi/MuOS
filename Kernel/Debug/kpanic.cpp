#include <Core/Log/Logger.hpp>
#include <Debug/kpanic.hpp>
#include <Memory/KHeap.hpp>

CREATE_LOGGER("assert", core::log::LogLevel::Debug);

[[noreturn]] void _kpanic_internal(const char* file, int line) {
	KHeap::instance().dump_stats();
	log.fatal("KERNEL PANIC at file: {}, line: {}", file, (int)line);
	log.fatal("System halted");
	while(true)
		asm volatile("cli\nhlt");
}