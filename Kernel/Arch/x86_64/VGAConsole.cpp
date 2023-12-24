#include <Arch/x86_64/VGA.hpp>
#include <Arch/x86_64/VGAConsole.hpp>
#include <Core/Error/Error.hpp>
#include <Core/Log/Logger.hpp>
#include <LibGeneric/LockGuard.hpp>

static vgacon::VgaConsole s_console {};

core::Error vgacon::init() {
	s_console.clear();
	return core::log::register_sink(&s_console);
}

void vgacon::VgaConsole::clear() {
	VGA::clear();
	VGA::setpos(0, 0);
	VGA::setcolor(VGA::VGA_COLOR_BLACK, VGA::VGA_COLOR_WHITE);
}

void vgacon::VgaConsole::push(core::log::LogLevel, const char* tag, const char* message) {
	gen::LockGuard lg { m_lock };

	VGA::putch('[');
	while(*tag != '\x0') {
		VGA::putch(*tag);
		++tag;
	}
	VGA::putch(']');
	VGA::putch(' ');

	while(*message != '\x0') {
		VGA::putch(*message);
		++message;
	}
}
