#pragma once
#include <SMP/ControlBlock.hpp>

class SMP {
public:
	static void bootstrap_ctb();
	static void init_control_blocks();
	static ControlBlock& ctb();
};