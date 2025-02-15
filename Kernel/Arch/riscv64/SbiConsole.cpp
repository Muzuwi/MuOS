#include <Arch/riscv64/SBI.hpp>
#include <Arch/riscv64/SbiConsole.hpp>
#include <Core/Error/Error.hpp>
#include <Core/Log/Logger.hpp>
#include <LibGeneric/Spinlock.hpp>
#include <SystemTypes.hpp>

///  FIXME: Move these to a single place, to be shared between x86's
///  SerialConsole and SbiConsole.
static const char RESET_FORMAT[] = "\x1b[0m\n";
static char const* prefix_for_level(core::log::LogLevel level) {
	switch(level) {
		case core::log::LogLevel::Debug: return "\x1b[90m";
		case core::log::LogLevel::Info: return "\x1b[37m";
		case core::log::LogLevel::Warning: return "\x1b[33m";
		case core::log::LogLevel::Error: return "\x1b[31m";
		case core::log::LogLevel::Fatal: return "\x1b[35m";
		default: return "";
	}
}

class SbiConsoleSink : public core::log::Sink {
public:
	constexpr SbiConsoleSink() = default;

	void push(core::log::LogLevel level, char const* tag, char const* message) override {
		//  Write the tag (subsystem the log is coming from).
		write_c_string("\x1b[34m[");
		write_c_string(tag);
		write_c_string("]\x1b[0m ");
		//  Write message contents, with some color formatting based on log level.
		write_c_string(prefix_for_level(level));
		write_c_string(message);
		write_c_string(RESET_FORMAT);
	}
private:
	void write_c_string(char const* str) {
		using namespace arch::rv64;
		while(str && *str != '\0') {
			sbi_call(static_cast<uint64>(*str), 0, 0, 0, 0, 0, SBI_FID_LEGACY, SBI_EID_PUTCHAR).ignore_error();
			++str;
		}
	}
};

static constinit SbiConsoleSink s_sink {};

core::Error arch::rv64::init_sbi_earlycon() {
	return core::log::register_sink(&s_sink);
}