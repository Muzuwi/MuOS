#include <Arch/i386/Registers.hpp>
#include <Kernel/Debug/kdebugf.hpp>
#include <Kernel/Process/Process.hpp>
#include <Kernel/Process/Scheduler.hpp>
#include <Kernel/Debug/kpanic.hpp>

/*
	This file contains exception handling routines for the kernel	
*/

void dump_reg_from_trap(const TrapFrame& regs) {
	kerrorf("eax: %x, ebx: %x, ecx: %x, edx: %x\n", regs.eax, regs.ebx, regs.ecx, regs.edx);
	kerrorf("ebp: %x, esp: %x, esi: %x, edi: %x\n", regs.ebp, (!Process::current() || (Process::current()->ring() == Ring::CPL0)) ? regs.handler_esp : regs.user_esp, regs.esi, regs.edi);
	kerrorf("eip: %x\n", regs.eip);
	kerrorf("Current IRQ trap frame: %x\n", Process::current()->irq_trap_frame());
}

void dump_reg_from_trap(const ErrorCodeTrapFrame& regs) {
	kerrorf("eax: %x, ebx: %x, ecx: %x, edx: %x\n", regs.eax, regs.ebx, regs.ecx, regs.edx);
	kerrorf("ebp: %x, esp: %x, esi: %x, edi: %x\n", regs.ebp, (!Process::current() || (Process::current()->ring() == Ring::CPL0)) ? regs.handler_esp : regs.user_esp, regs.esi, regs.edi);
	kerrorf("eip: %x\n", regs.eip);
	kerrorf("Current IRQ trap frame: %x\n", Process::current()->irq_trap_frame());
}

extern "C" void _kernel_exception_divbyzero(const TrapFrame& regs){
	kerrorf("Divide by zero exception at %x, KERNEL ABORT\n", regs.eip);

	kerrorf("System halted\n");
	asm volatile(
		"cli\n"
		"hlt\t\n"
		);
}


extern "C" void _kernel_exception_dbg(const TrapFrame&){
		
}


extern "C" void _kernel_exception_nmi(const TrapFrame&){
	
}


extern "C" void _kernel_exception_break(const TrapFrame&){
	
}


extern "C" void _kernel_exception_overflow(const TrapFrame&){
	
}


extern "C" void _kernel_exception_bound(const TrapFrame&){
	
}


extern "C" void _kernel_exception_invalidop(const TrapFrame& regs){
	bool is_kernel_crash = regs.eip >= (uint32_t)&_ukernel_virtual_offset;

	kerrorf("%s(%i): Illegal opcode exception at %x\n",
	        is_kernel_crash ? "Kernel" : "Process",
	        Process::current() ? Process::current()->pid() : 0,
	        regs.eip);
	dump_reg_from_trap(regs);

	if(is_kernel_crash) {
		kpanic();
	} else {
		Process::kill(Process::current()->pid());
		Scheduler::switch_task();
	}

}


extern "C" void _kernel_exception_nodevice(const TrapFrame&){

}


extern "C" void _kernel_exception_doublefault(const ErrorCodeTrapFrame& regs){
	kerrorf("Double Fault exception, ABORT\n");
	dump_reg_from_trap(regs);
	asm volatile(
	    "cli\n"
	    "hlt\t\n"
	    );
}


extern "C" void _kernel_exception_invalidtss(const ErrorCodeTrapFrame&){
	
}


extern "C" void _kernel_exception_invalidseg(const ErrorCodeTrapFrame& regs){
	asm volatile("cli\n");
	kerrorf("Segment not present fault at %x\n", regs.eip);
	dump_reg_from_trap(regs);
	asm volatile(
	"cli\n"
	"hlt\t\n"
	);
}


extern "C" void _kernel_exception_segstackfault(const ErrorCodeTrapFrame& regs){
	kerrorf("Stack segment fault at %x, KERNEL ABORT\n", regs.eip);
	dump_reg_from_trap(regs);
	kerrorf("Error code: %x\n", regs.error_code);
	asm volatile(
		"cli\n"
		"hlt\t\n"
		);

}


extern "C" void _kernel_exception_gpf(const ErrorCodeTrapFrame& regs){
	bool is_kernel_crash = regs.eip >= (uint32_t)&_ukernel_virtual_offset;
	kerrorf("%s(%i): General Protection Fault exception at %x\n",
	        is_kernel_crash ? "Kernel" : "Process",
	        Process::current() ? Process::current()->pid() : 0,
	        regs.eip);
	dump_reg_from_trap(regs);
	kerrorf("Error code: %x\n", regs.error_code);

	if(is_kernel_crash) {
		kpanic();
	} else {
		Process::kill(Process::current()->pid());
		Scheduler::switch_task();
	}
}


extern "C" void _kernel_exception_pagefault(const ErrorCodeTrapFrame& regs){
	asm volatile("cli\n");

	uint32_t cr2;
	asm volatile(
			"mov %0, cr2\n"
			:"=a"(cr2)
			);

	bool is_kernel_crash = !(regs.error_code & 4);

	kerrorf("%s(%i): Page Fault exception at %x\n",
			is_kernel_crash ? "Kernel" : "Process",
			Process::current() ? Process::current()->pid() : 0,
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
		Scheduler::switch_task();
	}

}


extern "C" void _kernel_exception_x87fpfault(const TrapFrame&){
	kpanic();
}


extern "C" void _kernel_exception_aligncheck(const ErrorCodeTrapFrame&){
	
}


extern "C" void _kernel_exception_machinecheck(const TrapFrame&){
	
}


extern "C" void _kernel_exception_simdfp(const TrapFrame&){
	
}


extern "C" void _kernel_exception_virtfault(const TrapFrame&){
	
}


extern "C" void _kernel_exception_security(const ErrorCodeTrapFrame&){
	
}
