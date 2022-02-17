#pragma once
#include <SystemTypes.hpp>

struct InactiveTaskFrame {
	uint64 rflags;
	uint64_t r15;
	uint64_t r14;
	uint64_t r13;
	uint64_t r12;
	uint64_t rbx;
	uint64_t rbp;
};