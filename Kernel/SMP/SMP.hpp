#pragma once

#include <SMP/ControlBlock.hpp>

class PhysAddr;

class SMP {
	static void ap_entrypoint(ControlBlock*, Thread*, PhysAddr, PhysAddr);

public:
	static void init_boot_ap_gdt(void*);

	static void bootstrap_ctb();

	static void init_control_blocks();

	static ControlBlock& ctb();
};