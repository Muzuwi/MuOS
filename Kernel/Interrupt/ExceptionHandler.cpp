#include <Arch/i386/PortIO.hpp>
#include <Arch/i386/Registers.hpp>
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
	asm volatile(
		"cli\n"
		"hlt\t\n"
		);
}


extern "C" void _kernel_exception_dbg(Registers& regs){
		
}


extern "C" void _kernel_exception_nmi(Registers& regs){
	
}


extern "C" void _kernel_exception_break(Registers& regs){
	
}


extern "C" void _kernel_exception_overflow(Registers& regs){
	
}


extern "C" void _kernel_exception_bound(Registers& regs){
	
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

}


extern "C" void _kernel_exception_nodevice(Registers& regs){

}


extern "C" void _kernel_exception_doublefault(Registers& regs){
	kerrorf("Double Fault exception, ABORT\n");
	kerrorf("eax: %x, ebx: %x, ecx: %x, edx: %x\n", regs.eax, regs.ebx, regs.ecx, regs.edx);
	kerrorf("ebp: %x, esp: %x, esi: %x, edi: %x\n", regs.ebp, regs.esp, regs.esi, regs.edi);
	kerrorf("eip: %x\n", regs.eip);
	asm volatile(
	    "cli\n"
	    "hlt\t\n"
	    );
}


extern "C" void _kernel_exception_invalidtss(Registers& regs){
	
}


extern "C" void _kernel_exception_invalidseg(Registers& regs){
	
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

}


extern "C" void _kernel_exception_gpf(Registers& regs){
	kerrorf("General Protection Fault exception at %x\n", regs.eip);
	kerrorf("eax: %x, ebx: %x, ecx: %x, edx: %x\n", regs.eax, regs.ebx, regs.ecx, regs.edx);
	kerrorf("ebp: %x, esp: %x, esi: %x, edi: %x\n", regs.ebp, regs.esp, regs.esi, regs.edi);
	kerrorf("eip: %x\n", regs.eip);
	kerrorf("Error code: %x", regs.error_code);
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
	kerrorf("Error code: %x\n", regs.error_code);
	kerrorf("Fault reason: %s - caused by a %s, CPL %i\n",
	        regs.error_code & 1 ? "Page-Protection violation" : "Non-present page",
	        regs.error_code & 2 ? "write" : "read",
	        regs.error_code & 4 ? 3 : 0
	        );
	asm volatile(
	    "cli\n"
	    "hlt\t\n"
	    );
}


extern "C" void _kernel_exception_x87fpfault(Registers& regs){
	
}


extern "C" void _kernel_exception_aligncheck(Registers& regs){
	
}


extern "C" void _kernel_exception_machinecheck(Registers& regs){
	
}


extern "C" void _kernel_exception_simdfp(Registers& regs){
	
}


extern "C" void _kernel_exception_virtfault(Registers& regs){
	
}


extern "C" void _kernel_exception_security(Registers& regs){
	
}
