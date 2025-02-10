#include <Arch/x86_64/MP/ExecutionEnvironment.hpp>
#include <Arch/x86_64/PIT.hpp>
#include <Arch/x86_64/Serial.hpp>
#include <Arch/x86_64/SerialConsole.hpp>
#include <Core/Error/Error.hpp>
#include <Core/Log/Logger.hpp>
#include <Core/MP/MP.hpp>
#include <LibFormat/Format.hpp>
#include <LibGeneric/LockGuard.hpp>

static serialcon::SerialConsole s_console {};

core::Error serialcon::init() {
	return core::log::register_sink(&s_console);
}

static void write_tag(char const* tag) {
	//  This is terrible, but it'll do for now to avoid changing everything
	Serial::write_debugger_str("\x1b[34m[");
	Serial::write_debugger_str(tag);
	Serial::write_debugger_str("]\x1b[0m ");
}

static void write_message(core::log::LogLevel level, char const* message) {
	auto prefix_for_level = [](core::log::LogLevel level) -> char const* {
		switch(level) {
			case core::log::LogLevel::Debug: return "\x1b[90m";
			case core::log::LogLevel::Info: return "\x1b[37m";
			case core::log::LogLevel::Warning: return "\x1b[33m";
			case core::log::LogLevel::Error: return "\x1b[31m";
			case core::log::LogLevel::Fatal: return "\x1b[35m";
			default: return "";
		}
	};

	Serial::write_debugger_str(prefix_for_level(level));
	Serial::write_debugger_str(message);
	Serial::write_debugger_str("\x1b[0m\n");
}

static void write_extras() {
	char buf[24];
	Format::format("\x1b[32m[+{}ms]", buf, sizeof(buf), PIT::milliseconds());
	Serial::write_debugger_str(buf);
	//  During early boot, we might not have GS base configured yet, and this_execution_environment
	//  would deref a nullptr.
	if(CPU::get_gs_base()) {
		Format::format("[~{}]\x1b[0m", buf, sizeof(buf), this_cpu()->platform.apic_id);
		Serial::write_debugger_str(buf);
	}
}

void serialcon::SerialConsole::push(core::log::LogLevel level, const char* tag, const char* message) {
	gen::LockGuard lg { m_lock };
	write_extras();
	write_tag(tag);
	write_message(level, message);
}
