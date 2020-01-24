#include <Arch/i386/PortIO.hpp>
#include <Arch/i386/VGA.h>
#include <Arch/i386/Timer.hpp>

/*
	Kernel Interrupt handler routines`
*/

extern "C" void _kernel_irq0_handler(void){
	Timer::getTimer().tick();
 	out(0x20, 0x20);	//  End of interrupt
 }

extern "C" void _kernel_irq1_handler(void){
	out(0x20, 0x20);	//  End of interrupt
}

extern "C" void _kernel_irq2_handler(void){
	out(0x20, 0x20);	//  End of interrupt
}

extern "C" void _kernel_irq3_handler(void){
	out(0x20, 0x20);	//  End of interrupt
}

extern "C" void _kernel_irq4_handler(void){
	out(0x20, 0x20);	//  End of interrupt
}

extern "C" void _kernel_irq5_handler(void){
	out(0x20, 0x20);	//  End of interrupt
}

extern "C" void _kernel_irq6_handler(void){
	out(0x20, 0x20);	//  End of interrupt
}

extern "C" void _kernel_irq7_handler(void){
	out(0x20, 0x20);	//  End of interrupt
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




