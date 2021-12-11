#include <string.h>
#include <stddef.h>
#include <Kernel/Debug/TTY.hpp>
#include <Arch/x86_64/VGA.hpp>
#include <Kernel/Locks/KMutex.hpp>
#include <Kernel/Process/Process.hpp>
#include <Kernel/SMP/SMP.hpp>

static KMutex s_tty_lock {};

void TTY::init(){
	VGA::clear();
	VGA::setpos(0,0);
	VGA::setcolor(VGA::VGA_COLOR_BLACK, VGA::VGA_COLOR_WHITE);
}

void TTY::prints(const char* data){
//	if(SMP::ctb().current_thread())
//		s_tty_lock.lock();

	for(size_t i = 0; i < strlen(data); i++)
		VGA::putch(data[i]);
	
//	if(SMP::ctb().current_thread())
//		s_tty_lock.unlock();
}

void TTY::printch(char data){
	VGA::putch(data);
}