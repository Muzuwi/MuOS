#include <Arch/x86_64/VGA.hpp>
#include <Core/MP/MP.hpp>
#include <Debug/TTY.hpp>
#include <Locks/KMutex.hpp>
#include <Process/Process.hpp>
#include <stddef.h>
#include <string.h>

static KMutex s_tty_lock {};

void TTY::init() {
	VGA::clear();
	VGA::setpos(0, 0);
	VGA::setcolor(VGA::VGA_COLOR_BLACK, VGA::VGA_COLOR_WHITE);
}

void TTY::prints(const char* data) {
	//	if(this_cpu()->current_thread())
	//		s_tty_lock.lock();

	for(size_t i = 0; i < strlen(data); i++)
		VGA::putch(data[i]);

	//	if(this_cpu()->current_thread())
	//		s_tty_lock.unlock();
}

void TTY::printch(char data) {
	VGA::putch(data);
}