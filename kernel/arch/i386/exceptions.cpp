#include <arch/i386/portio.hpp>
#include <arch/i386/registers.hpp>
#include <stdio.h>

/*
	This file contains exception handling routines for the kernel	
*/


extern "C" void _kernel_exception_divbyzero(Registers& regs){
	printf("Divide by zero exception at %x, KERNEL ABORT\n", regs.eip);
	printf("eax: %x, ebx: %x, ecx: %x, edx: %x\n", regs.eax, regs.ebx, regs.ecx, regs.edx);
	printf("ebp: %x, esp: %x, esi: %x, edi: %x\n", regs.ebp, regs.esp, regs.esi, regs.edi);
	printf("eip: %x\n", regs.eip);
	printf("System halted\n");
	out(0x20, 0x20);
	asm volatile(
		"cli\n"
		"hlt\t\n"
		);
}


extern "C" void _kernel_exception_dbg(Registers& regs){
	
	out(0x20, 0x20);
}


extern "C" void _kernel_exception_nmi(Registers& regs){
	
	out(0x20, 0x20);

}


extern "C" void _kernel_exception_break(Registers& regs){
	
	out(0x20, 0x20);

}


extern "C" void _kernel_exception_overflow(Registers& regs){
	
	out(0x20, 0x20);

}


extern "C" void _kernel_exception_bound(Registers& regs){
	
	out(0x20, 0x20);

}


extern "C" void _kernel_exception_invalidop(Registers& regs){
	
	out(0x20, 0x20);

}


extern "C" void _kernel_exception_nodevice(Registers& regs){
	
	out(0x20, 0x20);

}


extern "C" void _kernel_exception_doublefault(Registers& regs){
	printf("Double Fault exception, ABORT\n");
	printf("eax: %x, ebx: %x, ecx: %x, edx: %x\n", regs.eax, regs.ebx, regs.ecx, regs.edx);
	printf("ebp: %x, esp: %x, esi: %x, edi: %x\n", regs.ebp, regs.esp, regs.esi, regs.edi);
	printf("eip: %x\n", regs.eip);

	out(0x20, 0x20);
		asm volatile(
		"cli\n"
		"hlt\t\n"
		);
}


extern "C" void _kernel_exception_invalidtss(Registers& regs){
	
	out(0x20, 0x20);

}


extern "C" void _kernel_exception_invalidseg(Registers& regs){
	
	out(0x20, 0x20);

}


extern "C" void _kernel_exception_segstackfault(Registers& regs){
	
	out(0x20, 0x20);

}


extern "C" void _kernel_exception_gpf(Registers& regs){
	printf("General Protection Fault exception at %x\n", regs.eip);
	printf("eax: %x, ebx: %x, ecx: %x, edx: %x\n", regs.eax, regs.ebx, regs.ecx, regs.edx);
	printf("ebp: %x, esp: %x, esi: %x, edi: %x\n", regs.ebp, regs.esp, regs.esi, regs.edi);
	printf("eip: %x\n", regs.eip);
	out(0x20, 0x20);
	asm volatile(
		"cli\n"
		"hlt\t\n"
		);
}


extern "C" void _kernel_exception_pagefault(Registers& regs){
	
	out(0x20, 0x20);

}


extern "C" void _kernel_exception_x87fpfault(Registers& regs){
	
	out(0x20, 0x20);

}


extern "C" void _kernel_exception_aligncheck(Registers& regs){
	
	out(0x20, 0x20);

}


extern "C" void _kernel_exception_machinecheck(Registers& regs){
	
	out(0x20, 0x20);

}


extern "C" void _kernel_exception_simdfp(Registers& regs){
	
	out(0x20, 0x20);

}


extern "C" void _kernel_exception_virtfault(Registers& regs){
	
	out(0x20, 0x20);

}


extern "C" void _kernel_exception_security(Registers& regs){
	
	out(0x20, 0x20);

}