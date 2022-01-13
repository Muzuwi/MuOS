#pragma once
#include <Kernel/SystemTypes.hpp>

//  This structure must be kept in sync with the one in V86.asm
struct V86Regs {
	uint16 ax;
	uint16 bx;
	uint16 cx;
	uint16 dx;
	uint16 si;
	uint16 di;
	uint16 es;
} __attribute__((packed));

class V86 {
public:
	static void run_irq(uint8 irq, V86Regs& regs);
};
