#include <Arch/VM.hpp>
#include <Arch/x86_64/VGA.hpp>
#include <stddef.h>
#include <string.h>

static uint8_t vga_console_position_x = 0, vga_console_position_y = 0;
static uint8_t vga_color = 0;

static PhysPtr<uint16_t> vga_buffer_base_address { reinterpret_cast<uint16_t*>(0xB8000) };

//  Set console color byte
void VGA::setcolor(enum VGA_COLOR bg, enum VGA_COLOR fg) {
	vga_color = (bg << 4) | (fg);
}

//  Get color byte
inline static uint8_t _getcolor(VGA::VGA_COLOR bg, VGA::VGA_COLOR fg) {
	return (bg << 4) | (fg);
}

//  Moves the buffer contents upwards by n lines, while filling the empty spaces created with zeroes
inline static void vga_move_buffer(size_t lines) {
	for(size_t i = 0; i < VGA::VGA_HEIGHT; i++) {
		for(size_t j = 0; j < VGA::VGA_WIDTH; j++) {
			if(i < VGA::VGA_HEIGHT - lines) {
				vga_buffer_base_address[i * VGA::VGA_WIDTH + j] =
				        vga_buffer_base_address[(i + lines) * VGA::VGA_WIDTH + j];
			} else {
				vga_buffer_base_address[i * VGA::VGA_WIDTH + j] = 0;
			}
		}
	}
}

//  Handles newline behaviour
inline static void newline() {
	vga_console_position_x = 0;
	if(++vga_console_position_y == VGA::VGA_HEIGHT) {
		vga_move_buffer(1);
		vga_console_position_y = VGA::VGA_HEIGHT - 1;
	}
}

//  Prints a character
void VGA::putch(char ch) {
	if(ch == '\n') {
		newline();
		return;
	}
	vga_buffer_base_address[vga_console_position_y * VGA::VGA_WIDTH + vga_console_position_x] =
	        (vga_color << 8) | (ch & 0xFF);
	if(++vga_console_position_x == VGA::VGA_WIDTH) {
		newline();
	}
}

//  Clears the VGA Buffer (all black)
void VGA::clear() {
	for(size_t y = 0; y < VGA_WIDTH * VGA_HEIGHT; y++) {
		vga_buffer_base_address[y] = 0;
	}
}

void VGA::setpos(size_t x, size_t y) {
	vga_console_position_x = x;
	vga_console_position_y = y;
}
