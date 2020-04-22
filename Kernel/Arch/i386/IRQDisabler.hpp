#pragma once

class IRQDisabler {
	bool clearOnExit;
public:
	IRQDisabler() {
		unsigned long flags;
		asm volatile("pushf\n"
			         "pop %0\n"
		: "=rm"(flags)
		:
		: "memory"
			   );

		clearOnExit = flags & (0x200);

		asm volatile("cli\n");
	}

	~IRQDisabler() {
		if(clearOnExit)
			asm volatile("sti\n");
	}
};