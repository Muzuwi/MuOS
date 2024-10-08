#include <Arch/Interrupt.hpp>
#include <Core/Assert/Assert.hpp>
#include <Core/Assert/Panic.hpp>
#include <Core/Log/Logger.hpp>

CREATE_LOGGER("panic", core::log::LogLevel::Debug);

[[noreturn]] void core::panic(char const* message) {
	::log.fatal("PANIC: {}\n", message);
	irq_local_disable();
	HANG();
}

void core::warn(char const* messsage) {
	::log.warning("WARN: {}\n", messsage);
}