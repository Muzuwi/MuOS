#include <Arch/i386/portio.hpp>
#include <Arch/i386/registers.hpp>
#include <Kernel/Debug/kdebugf.hpp>

/*
	This file contains exception handling routines for the kernel	
*/


extern "C" void _kernel_exception_divbyzero(Registers& regs){
	kerrorf("Divide by zero exception at %x, KERNEL ABORT\n", regs.eip);
	kerrorf("eax: %x, ebx: %x, ecx: %x, edx: %x\n", regs.eax, regs.ebx, regs.ecx, regs.edx);
	kerrorf("ebp: %x, esp: %x, esi: %x, edi: %x\n", regs.ebp, regs.esp, regs.esi, regs.edi);
	kerrorf("eip: %x\n", regs.eip);
	kerrorf("System halted\n");
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
	kerrorf("Invalid opcode exception at %x, KERNEL ABORT\n", regs.eip);
	kerrorf("eax: %x, ebx: %x, ecx: %x, edx: %x\n", regs.eax, regs.ebx, regs.ecx, regs.edx);
	kerrorf("ebp: %x, esp: %x, esi: %x, edi: %x\n", regs.ebp, regs.esp, regs.esi, regs.edi);
	kerrorf("eip: %x\n", regs.eip);
	kerrorf("Offending instruction: %x", *((uint32_t*)regs.eip));
	kerrorf("System halted\n");
	asm volatile(
		"cli\n"
		"hlt\t\n"
		);

	out(0x20, 0x20);
}


extern "C" void _kernel_exception_nodevice(Registers& regs){

	out(0x20, 0x20);
}


extern "C" void _kernel_exception_doublefault(Registers& regs){
	kerrorf("Double Fault exception, ABORT\n");
	kerrorf("eax: %x, ebx: %x, ecx: %x, edx: %x\n", regs.eax, regs.ebx, regs.ecx, regs.edx);
	kerrorf("ebp: %x, esp: %x, esi: %x, edi: %x\n", regs.ebp, regs.esp, regs.esi, regs.edi);
	kerrorf("eip: %x\n", regs.eip);

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
	kerrorf("Stack segment fault at %x, KERNEL ABORT\n", regs.eip);
	kerrorf("eax: %x, ebx: %x, ecx: %x, edx: %x\n", regs.eax, regs.ebx, regs.ecx, regs.edx);
	kerrorf("ebp: %x, esp: %x, esi: %x, edi: %x\n", regs.ebp, regs.esp, regs.esi, regs.edi);
	kerrorf("eip: %x\n", regs.eip);
	kerrorf("Error code: %x\n", regs.error_code);
	asm volatile(
		"cli\n"
		"hlt\t\n"
		);

	out(0x20, 0x20);
}


extern "C" void _kernel_exception_gpf(Registers& regs){
	kerrorf("General Protection Fault exception at %x\n", regs.eip);
	kerrorf("eax: %x, ebx: %x, ecx: %x, edx: %x\n", regs.eax, regs.ebx, regs.ecx, regs.edx);
	kerrorf("ebp: %x, esp: %x, esi: %x, edi: %x\n", regs.ebp, regs.esp, regs.esi, regs.edi);
	kerrorf("eip: %x\n", regs.eip);
	kerrorf("Error code: %x", regs.error_code);
	out(0x20, 0x20);
	asm volatile(
		"cli\n"
		"hlt\t\n"
		);
}


extern "C" void _kernel_exception_pagefault(Registers& regs){
	kerrorf("Page Fault exception at %x\n", regs.eip);
	kerrorf("eax: %x, ebx: %x, ecx: %x, edx: %x\n", regs.eax, regs.ebx, regs.ecx, regs.edx);
	kerrorf("ebp: %x, esp: %x, esi: %x, edi: %x\n", regs.ebp, regs.esp, regs.esi, regs.edi);
	kerrorf("eip: %x\n", regs.eip);
	kerrorf("Error code: %x", regs.error_code);
	out(0x20, 0x20);
	asm volatile(
		"cli\n"
		"hlt\t\n"
		);
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