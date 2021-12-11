#pragma once
#include <Kernel/SystemTypes.hpp>

class Thread;
struct PtraceRegs;

class CPU {
public:
	static void initialize_features();
	static void switch_to(Thread* prev, Thread* next);
	static void irq_disable();
	static void irq_enable();
	static void set_kernel_gs_base(void*);
	static uint64_t get_kernel_gs_base();
	static uint64_t get_gs_base();
	static uint64   cr2();
	static uint64   cr3();
	static void set_gs_base(void*);

	[[noreturn]]
	static void jump_to_user(PtraceRegs* regs);
};