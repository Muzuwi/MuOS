#include <Debug/DebugCon.hpp>
#include <Debug/TTY.hpp>
#include <Device/Serial.hpp>

void Debug::log_info(char const* message) {
	Serial::write_debugger_str("\u001b[32m");
	Serial::write_debugger_str(message);
	Serial::write_debugger_str("\u001b[0m");
	TTY::prints(message);
}

void Debug::log_error(char const* message) {
	Serial::write_debugger_str("\u001b[1;91m");
	Serial::write_debugger_str(message);
	Serial::write_debugger_str("\u001b[0m");
	TTY::prints(message);
}
