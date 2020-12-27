#include <string.h>
#include <stddef.h>
#include <Kernel/Debug/TTY.hpp>
#include <Arch/i386/VGA.hpp>

void TTY::init(){
	VGA::clear();
	VGA::setpos(0,0);
	VGA::setcolor(VGA::VGA_COLOR_BLACK, VGA::VGA_COLOR_WHITE);
}

void TTY::prints(const char* data){
	for(size_t i = 0; i < strlen(data); i++)
		VGA::putch(data[i]);
}

void TTY::printch(char data){
	VGA::putch(data);
}