#pragma once
#include <Arch/x86_64/CPU.hpp>

class IRQDisabler final {
	uint64 m_previous;
public:
	IRQDisabler() noexcept {
		asm volatile("pushf  \t\n"
		             "pop %0 \t\n"
		             : "=rm"(m_previous)
		             :
		             : "memory");
		if(m_previous & 0x0200) {
			CPU::irq_disable();
		}
	}

	~IRQDisabler() {
		if(m_previous & 0x0200) {
			CPU::irq_enable();
		}
	}
};