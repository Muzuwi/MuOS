#include <Arch/i386/CPU.hpp>
#include <Kernel/Debug/kpanic.hpp>
#include <Arch/i386/GDT.hpp>
#include <Kernel/Process/Process.hpp>
#include <Kernel/Debug/kassert.hpp>

void CPU::initialize_features() {
	asm volatile(
	"wrmsr"
	:: "a"(1), "d"(0), "c"(0xC0000080)
	);

	//  FPU initialization
	uint16_t x37f = 0x37f;
	uint16_t x37e = 0x37e;
	uint16_t x37a = 0x37a;
	asm volatile(
			"mov %%eax, %%cr0\n"
            "and %%eax, 0xfffffffb\n"
			"or %%eax, 0x20\n"
			"mov %%cr0, %%eax\n"
			"fninit\n"
            "fldcw %0\n"
			"fldcw %1\n"
			"fldcw %2\n"
	:
	:"m"(x37f), "m"(x37e), "m"(x37a)
	:"eax" );
}

/*
 *  Restores CPU register state from a trap frame, and resumes execution in Ring3
 */
void CPU::jump_to_trap_ring3(TrapFrame trap) {
	ASSERT_IRQ_DISABLED();
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
	""(trap._user_esp),
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
void CPU::jump_to_trap_ring0(TrapFrame trap) {
	ASSERT_IRQ_DISABLED();
	asm volatile(
        "mov %%esp, %9\n"
		"push %6\n"     //  EIP
        "push %8\n"     //  EFLAGS
        "push %0\n"     //  EAX
		"push %2\n"     //  ECX
		"push %3\n"     //  EDX
		"push %1\n"     //  EBX
		"push 0\n"     //  unused
		"push %10\n"     //  EBP
		"push %5\n"     //  ESI
		"push %4\n"     //  EDI

		"popad\n"
        "popfd\n"
        "ret\n"
	:
	: ""(trap.eax), ""(trap.ebx), ""(trap.ecx),
	  ""(trap.edx), ""(trap.edi), ""(trap.esi),
	  ""(trap.eip), ""(trap.CS), ""(trap.EFLAGS),
	  ""(trap._user_esp), ""(trap.ebp)
	: "eax", "ebx", "ecx", "edx", "edi", "esi"
	);
	kpanic();
}

void CPU::load_segment_registers_for(Ring r) {
	asm volatile(
	"mov %%ds, %0\n"
	"mov %%es, %0\n"
	"mov %%fs, %0\n"
	"mov %%gs, %0\n"
	::"r"((r == Ring::CPL3 ? (GDT::get_user_DS() | 3) : GDT::get_kernel_DS()))
	);
}
