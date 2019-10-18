#include <arch/i386/portio.hpp>
#include <arch/i386/vga.h>
#include <stdio.h>

/*
	Kernel Interrupt handler routines`
*/

extern "C" void _kernel_irq0_handler(void){
	vga_putdebugch(VGA_WIDTH - 1, VGA_HEIGHT - 1, '0', 4, 15);
	out(0x20, 0x20);	//  End of interrupt
	vga_putdebugch(VGA_WIDTH - 1, VGA_HEIGHT - 1, '0', 4, 4);
}

extern "C" void _kernel_irq1_handler(void){
	vga_putdebugch(VGA_WIDTH - 2, VGA_HEIGHT - 1, '1', 4, 15);
	//printf("[_kernel_irq1_handler] int\n");
	out(0x20, 0x20);	//  End of interrupt
	vga_putdebugch(VGA_WIDTH - 2, VGA_HEIGHT - 1, '1', 4, 4);
	//vga_putdebugch(VGA_WIDTH - 2, VGA_HEIGHT, '0', 4, 4);
}

extern "C" void _kernel_irq2_handler(void){
	vga_putdebugch(VGA_WIDTH - 3, VGA_HEIGHT - 1, '2', 4, 15);
	printf("[_kernel_irq2_handler] int\n");
	out(0x20, 0x20);	//  End of interrupt
	vga_putdebugch(VGA_WIDTH - 3, VGA_HEIGHT - 1, '2', 4, 4);
}

extern "C" void _kernel_irq3_handler(void){
	vga_putdebugch(VGA_WIDTH - 4, VGA_HEIGHT - 1, '3', 4, 15);
	out(0x20, 0x20);	//  End of interrupt
	vga_putdebugch(VGA_WIDTH - 4, VGA_HEIGHT - 1, '3', 4, 4);
}

extern "C" void _kernel_irq4_handler(void){
	vga_putdebugch(VGA_WIDTH - 5, VGA_HEIGHT - 1, '4', 4, 15);
	out(0x20, 0x20);	//  End of interrupt
	vga_putdebugch(VGA_WIDTH - 5, VGA_HEIGHT - 1, '4', 4, 4);
}

extern "C" void _kernel_irq5_handler(void){
	vga_putdebugch(VGA_WIDTH - 6, VGA_HEIGHT - 1, '5', 4, 15);
	out(0x20, 0x20);	//  End of interrupt
	vga_putdebugch(VGA_WIDTH - 6, VGA_HEIGHT - 1, '5', 4, 4);
}

extern "C" void _kernel_irq6_handler(void){
	vga_putdebugch(VGA_WIDTH - 7, VGA_HEIGHT - 1, '6', 4, 15);
	out(0x20, 0x20);	//  End of interrupt
	vga_putdebugch(VGA_WIDTH - 7, VGA_HEIGHT - 1, '6', 4, 4);
}

extern "C" void _kernel_irq7_handler(void){
	vga_putdebugch(VGA_WIDTH - 8, VGA_HEIGHT - 1, '7', 4, 15);
	out(0x20, 0x20);	//  End of interrupt
	vga_putdebugch(VGA_WIDTH - 8, VGA_HEIGHT - 1, '7', 4, 4);
}

extern "C" void _kernel_irq8_handler(void){
	out(0xA0, 0x20);
	out(0x20, 0x20);	//  End of interrupt
}

extern "C" void _kernel_irq9_handler(void){
	out(0xA0, 0x20);
	out(0x20, 0x20);	//  End of interrupt
}

extern "C" void _kernel_irq10_handler(void){
	out(0xA0, 0x20);
	out(0x20, 0x20);	//  End of interrupt
}

extern "C" void _kernel_irq11_handler(void){
	out(0xA0, 0x20);
	out(0x20, 0x20);	//  End of interrupt
}

extern "C" void _kernel_irq12_handler(void){
	out(0xA0, 0x20);
	out(0x20, 0x20);	//  End of interrupt
}

extern "C" void _kernel_irq13_handler(void){
	out(0xA0, 0x20);
	out(0x20, 0x20);	//  End of interrupt
}

extern "C" void _kernel_irq14_handler(void){
	out(0xA0, 0x20);
	out(0x20, 0x20);	//  End of interrupt
}

extern "C" void _kernel_irq15_handler(void){
	out(0xA0, 0x20);
	out(0x20, 0x20);	//  End of interrupt
}




