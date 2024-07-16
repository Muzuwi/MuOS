#pragma once
#include <SystemTypes.hpp>

#define irq_local_enable()              \
	do {                                \
		asm volatile("sti" : : : "cc"); \
	} while(false)

#define irq_local_disable()             \
	do {                                \
		asm volatile("cli" : : : "cc"); \
	} while(false)

static inline bool _irq_local_enabled() {
	uint64 data;
	asm volatile("pushf  \t\n"
	             "pop %0 \t\n"
	             : "=rm"(data)
	             :
	             : "memory");
	return data & 0x0200;
}

#define irq_local_enabled() (_irq_local_enabled())
