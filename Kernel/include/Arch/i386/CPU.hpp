#pragma once
#include <Kernel/SystemTypes.hpp>

class Process;

class CPU {
public:
	static void initialize_features();
	static void switch_to(Process* prev, Process* next);
	static void irq_disable();
	static void irq_enable();
	static void set_kernel_gs_base(void*);
	static uint64_t get_kernel_gs_base();
	static void set_gs_base(void*);
};