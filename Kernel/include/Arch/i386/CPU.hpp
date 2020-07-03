#pragma once
#include <Arch/i386/TrapFrame.hpp>

enum class Ring;

namespace CPU {
	void initialize_features();

	[[noreturn]] void jump_to_trap_ring3(TrapFrame);
	[[noreturn]] void jump_to_trap_ring0(TrapFrame);
	[[noreturn]] void jump_to_irq_frame(void* irq_frame, void* page_directory);

	void load_segment_registers_for(Ring);
}