#include <string.h>
#include <Arch/i386/vga.h>
#include <Kernel/Symbols.hpp>

static uint16_t* vga_buffer_base_address = (uint16_t*)(0xB8000);

uint8_t vga_console_position_x = 0, vga_console_position_y = 0;
uint8_t vga_color = 0;

//  Set a character at x y without affecting the current console position
void vga_putdebugch(size_t x, size_t y, char ch, uint8_t bg, uint8_t fg){
	if(x >= VGA_WIDTH || y >= VGA_HEIGHT || x < 0 || y < 0){
		return;
	}

	vga_buffer_base_address[y*VGA_WIDTH + x] = (((bg << 4) | fg) << 8) | (ch & 0xFF);
}

//  Set console color byte
void vga_setcolor(enum VGA_COLOR bg, enum VGA_COLOR fg){
	vga_color = (bg << 4) | (fg);
}

//  Get color byte
inline static uint8_t vga_getcolor(enum VGA_COLOR bg, enum VGA_COLOR fg){
	return (bg << 4) | (fg);
}

//  Moves the buffer contents upwards by n lines, while filling the empty spaces created with zeroes
inline static void vga_move_buffer(size_t lines){
	for(size_t i = 0; i < VGA_HEIGHT; i++){
		for(size_t j = 0; j < VGA_WIDTH; j++){
			//vga_buffer_base_address[(i-lines)*VGA_WIDTH + j] = vga_buffer_base_address[(i)*VGA_WIDTH + j];
			//vga_buffer_base_address[(i-lines)*VGA_WIDTH + j] = 0;
			if(i <  VGA_HEIGHT - lines){
				vga_buffer_base_address[i*VGA_WIDTH + j] = vga_buffer_base_address[(i+lines)*VGA_WIDTH + j];
			} else {
				vga_buffer_base_address[i*VGA_WIDTH + j] = 0;
			}
		}
	}
}


//  Handles newline behaviour
inline static void vga_newline(){
	vga_console_position_x = 0;
	if(++vga_console_position_y == VGA_HEIGHT){
		vga_move_buffer(1);
		vga_console_position_y = VGA_HEIGHT - 1;
	}		
}

//  Prints a character
void vga_putch(char ch){
	if(ch == '\n'){
		vga_newline();
		return;
	}
	vga_buffer_base_address[vga_console_position_y*VGA_WIDTH + vga_console_position_x] = (vga_color << 8) | (ch & 0xFF);
	if(++vga_console_position_x == VGA_WIDTH) {
		vga_newline();
	}
}

//  Clears the VGA Buffer (all black)
void vga_clear(void){
	for(size_t y = 0; y < VGA_WIDTH*VGA_HEIGHT; y++){
		vga_buffer_base_address[y] = 0;
	}
}

void vga_setpos(size_t x, size_t y){
	vga_console_position_x = x;
	vga_console_position_y = y;
}

void vga_set_buffer(){
	vga_buffer_base_address = (uint16_t*)(0xB8000 + (uint32_t)&_ukernel_virtual_offset);
}