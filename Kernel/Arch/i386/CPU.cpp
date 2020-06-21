#include <Arch/i386/CPU.hpp>
#include <Kernel/Debug/kpanic.hpp>
#include <Arch/i386/GDT.hpp>


void CPU::initialize_features() {
	asm volatile(
	"wrmsr"
	:: "a"(1), "d"(0), "c"(0xC0000080)
	);
}

/*
 *  Restores CPU register state from a trap frame, and resumes execution in Ring3
 */
void CPU::jump_to_trap_ring3(const TrapFrame& trap) {
	asm volatile(
		"push %0\n"     //  SS
		"push %2\n"     //  ESP
		"push %9\n"     //  EFLAGS
		"push %1\n"     //  CS
		"push %10\n"    //  EIP

        "push %3\n"     //  EAX
		"push %5\n"     //  ECX
		"push %6\n"     //  EDX
		"push %4\n"     //  EBX
		"push 0\n"      //  ESP (skipped by POPAD)
		"push %11\n"    //  EBP
		"push %8\n"     //  ESI
		"push %7\n"     //  EDI
        "popad\n"

		"iret\n"
	:
	:""(GDT::get_user_DS() | 3), ""(GDT::get_user_CS() | 3),
	""(trap.user_esp),
	""(trap.eax), ""(trap.ebx), ""(trap.ecx),
	""(trap.edx), ""(trap.edi), ""(trap.esi),
	""(trap.EFLAGS),""(trap.eip), ""(trap.ebp)
	: "eax", "ebx", "ecx", "edx", "edi", "esi"
	);
	kpanic();
}

/*
 *  Restores CPU register state from a trap frame, and resumes execution in Ring0
 *  Currently unused, but might be used for kernel tasks. Will also be rewritten
 *  at that point, this most likely doesn't even work
 */
void CPU::jump_to_trap_ring0(const TrapFrame& trap) {
	asm volatile(
	"mov %%eax, %0\n"
	"mov %%ebx, %1\n"
	"mov %%ecx, %2\n"
	"mov %%edx, %3\n"
	"mov %%edi, %4\n"
	"mov %%esi, %5\n"

	"mov %%esp, %9\n"
	"add %%esp, 12\n"
	"push %8\n"
	"push %7\n"
	"push %6\n"
	"mov %%ebp, %10\n"
	"iret"
	:
	: ""(trap.eax), ""(trap.ebx), ""(trap.ecx), ""(trap.edx), ""(trap.edi), ""(trap.esi),
	""(trap.eip), ""(trap.CS), ""(trap.EFLAGS), ""(trap.handler_esp), ""(trap.ebp)
	: "eax", "ebx", "ecx", "edx", "edi", "esi"
	);
	kpanic();
}
