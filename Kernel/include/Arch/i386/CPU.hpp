#pragma once
#include <Arch/i386/TrapFrame.hpp>

namespace CPU {
	void initialize_features();

	[[noreturn]] void jump_to_trap_ring3(const TrapFrame&);
	[[noreturn]] void jump_to_trap_ring0(const TrapFrame&);
}