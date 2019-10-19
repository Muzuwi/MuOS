#include <arch/i386/portio.hpp>
#include <stdio.h>

/*
	This file contains exception handling routines for the kernel	
*/


extern "C" void _kernel_exception_divbyzero( uint32_t fault_addr ){
	printf("Divide by zero exception at %x, KERNEL ABORT\n", fault_addr);
	out(0x20, 0x20);
	asm volatile(
		"cli\n"
		"hlt\t\n"
		);
}


extern "C" void _kernel_exception_dbg(){
	
	out(0x20, 0x20);
}


extern "C" void _kernel_exception_nmi(){
	
	out(0x20, 0x20);

}


extern "C" void _kernel_exception_break(){
	
	out(0x20, 0x20);

}


extern "C" void _kernel_exception_overflow(){
	
	out(0x20, 0x20);

}


extern "C" void _kernel_exception_bound(){
	
	out(0x20, 0x20);

}


extern "C" void _kernel_exception_invalidop(){
	
	out(0x20, 0x20);

}


extern "C" void _kernel_exception_nodevice(){
	
	out(0x20, 0x20);

}


extern "C" void _kernel_exception_doublefault(){
	printf("Double Fault exception, ABORT\n");
	out(0x20, 0x20);
		asm volatile(
		"cli\n"
		"hlt\t\n"
		);
}


extern "C" void _kernel_exception_invalidtss(){
	
	out(0x20, 0x20);

}


extern "C" void _kernel_exception_invalidseg(){
	
	out(0x20, 0x20);

}


extern "C" void _kernel_exception_segstackfault(){
	
	out(0x20, 0x20);

}


extern "C" void _kernel_exception_gpf(uint32_t fault_addr){
	printf("General Protection Fault exception at %x\n", fault_addr);
	out(0x20, 0x20);
	asm volatile(
		"cli\n"
		"hlt\t\n"
		);
}


extern "C" void _kernel_exception_pagefault(){
	
	out(0x20, 0x20);

}


extern "C" void _kernel_exception_x87fpfault(){
	
	out(0x20, 0x20);

}


extern "C" void _kernel_exception_aligncheck(){
	
	out(0x20, 0x20);

}


extern "C" void _kernel_exception_machinecheck(){
	
	out(0x20, 0x20);

}


extern "C" void _kernel_exception_simdfp(){
	
	out(0x20, 0x20);

}


extern "C" void _kernel_exception_virtfault(){
	
	out(0x20, 0x20);

}


extern "C" void _kernel_exception_security(){
	
	out(0x20, 0x20);

}