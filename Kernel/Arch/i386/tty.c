#include <stdarg.h>
#include <string.h>
#include <Arch/i386/vga.h>

void tty_init(){
    vga_clear();
    vga_setpos(0,0);
    vga_setcolor(VGA_COLOR_BLACK, VGA_COLOR_WHITE);
	vga_set_buffer();
}

void tty_prints(const char* data){
    for(size_t i = 0; i < strlen(data); i++) 
        vga_putch(data[i]);
}

void tty_printch(char data){
    vga_putch(data);
}