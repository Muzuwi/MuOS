#include <Arch/i386/Registers.hpp>
#include <Kernel/Debug/kdebugf.hpp>
#include <Kernel/Process/Process.hpp>
#include <Kernel/Process/Scheduler.hpp>
#include <Kernel/Debug/kpanic.hpp>

/*
	This file contains exception handling routines for the kernel	
*/

void dump_reg_from_trap(TrapFrame regs) {
	kerrorf("eax: %x, ebx: %x, ecx: %x, edx: %x\n", regs.eax, regs.ebx, regs.ecx, regs.edx);
	kerrorf("ebp: %x, esp: %x, esi: %x, edi: %x\n", regs.ebp, !Process::current() || Process::current()->ring() == Ring::CPL0 ? regs.handler_esp : regs.user_esp, regs.esi, regs.edi);
	kerrorf("eip: %x\n", regs.eip);
}

void dump_reg_from_trap(ErrorCodeTrapFrame regs) {
	kerrorf("eax: %x, ebx: %x, ecx: %x, edx: %x\n", regs.eax, regs.ebx, regs.ecx, regs.edx);
	kerrorf("ebp: %x, esp: %x, esi: %x, edi: %x\n", regs.ebp, !Process::current() || Process::current()->ring() == Ring::CPL0 ? regs.handler_esp : regs.user_esp, regs.esi, regs.edi);
	kerrorf("eip: %x\n", regs.eip);
}

extern "C" void _kernel_exception_divbyzero(TrapFrame regs){
	kerrorf("Divide by zero exception at %x, KERNEL ABORT\n", regs.eip);

	kerrorf("System halted\n");
	asm volatile(
		"cli\n"
		"hlt\t\n"
		);
}


extern "C" void _kernel_exception_dbg(TrapFrame){
		
}


extern "C" void _kernel_exception_nmi(TrapFrame){
	
}


extern "C" void _kernel_exception_break(TrapFrame){
	
}


extern "C" void _kernel_exception_overflow(TrapFrame){
	
}


extern "C" void _kernel_exception_bound(TrapFrame){
	
}


extern "C" void _kernel_exception_invalidop(TrapFrame regs){
	kerrorf("Invalid opcode exception at %x, KERNEL ABORT\n", regs.eip);
	dump_reg_from_trap(regs);
	kerrorf("Offending instruction: %x", *((uint32_t*)regs.eip));
	kerrorf("System halted\n");
	asm volatile(
		"cli\n"
		"hlt\t\n"
		);

}


extern "C" void _kernel_exception_nodevice(TrapFrame){

}


extern "C" void _kernel_exception_doublefault(ErrorCodeTrapFrame regs){
	kerrorf("Double Fault exception, ABORT\n");
	dump_reg_from_trap(regs);
	asm volatile(
	    "cli\n"
	    "hlt\t\n"
	    );
}


extern "C" void _kernel_exception_invalidtss(ErrorCodeTrapFrame){
	
}


extern "C" void _kernel_exception_invalidseg(ErrorCodeTrapFrame regs){
	asm volatile("cli\n");
	kerrorf("Segment not present fault at %x\n", regs.eip);
	dump_reg_from_trap(regs);
	asm volatile(
	"cli\n"
	"hlt\t\n"
	);
}


extern "C" void _kernel_exception_segstackfault(ErrorCodeTrapFrame regs){
	kerrorf("Stack segment fault at %x, KERNEL ABORT\n", regs.eip);
	dump_reg_from_trap(regs);
	kerrorf("Error code: %x\n", regs.error_code);
	asm volatile(
		"cli\n"
		"hlt\t\n"
		);

}


extern "C" void _kernel_exception_gpf(ErrorCodeTrapFrame regs){
	asm volatile("cli\n");

	bool is_kernel_crash = regs.eip >= (uint32_t)&_ukernel_virtual_offset;
	kerrorf("%s(%i): General Protection Fault exception at %x\n",
	        is_kernel_crash ? "Kernel" : "Process",
	        is_kernel_crash ? 0 : Process::current()->pid(),
	        regs.eip);
	dump_reg_from_trap(regs);
	kerrorf("Error code: %x\n", regs.error_code);

	if(is_kernel_crash) {
		kpanic();
	} else {
		Process::kill(Process::current()->pid());
		Scheduler::yield({});
	}
}


extern "C" void _kernel_exception_pagefault(ErrorCodeTrapFrame regs){
	asm volatile("cli\n");

	uint32_t cr2;
	asm volatile(
			"mov %0, cr2\n"
			:"=a"(cr2)
			);

	bool is_kernel_crash = !(regs.error_code & 4);

	kerrorf("%s(%i): Page Fault exception at %x\n",
			is_kernel_crash ? "Kernel" : "Process",
			is_kernel_crash ? 0 : Process::current()->pid(),
			regs.eip);
	dump_reg_from_trap(regs);
	kerrorf("Error code: %x [%s - caused by a %s, CPL %i]\n", regs.error_code,
	        regs.error_code & 1 ? "Page-Protection violation" : "Non-present page",
	        regs.error_code & 2 ? "write" : "read",
	        regs.error_code & 4 ? 3 : 0);
	if(cr2 == 0) {
		kerrorf("Notice: Possible null pointer dereference - tried accessing address 0\n");
	} else if(cr2 == 0x99) {
		kerrorf("Notice: Possible uninitialized memory - tried accessing address 0x99 (KM sanitizer byte)\n");
	}

	//  Ring 0 violation
	if(is_kernel_crash) {
		kpanic();
	} else { //  Ring 3
		Process::kill(Process::current()->pid());
		Scheduler::yield({});
	}

}


extern "C" void _kernel_exception_x87fpfault(TrapFrame){
	
}


extern "C" void _kernel_exception_aligncheck(ErrorCodeTrapFrame){
	
}


extern "C" void _kernel_exception_machinecheck(TrapFrame){
	
}


extern "C" void _kernel_exception_simdfp(TrapFrame){
	
}


extern "C" void _kernel_exception_virtfault(TrapFrame){
	
}


extern "C" void _kernel_exception_security(ErrorCodeTrapFrame){
	
}
