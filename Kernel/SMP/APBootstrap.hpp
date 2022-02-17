#pragma once

#include <SystemTypes.hpp>

class ControlBlock;

class Thread;

//  Keep this in sync with the one in APBoot.asm
struct APBoostrap {
	uint64 state_flag;
	uint64 cr3;
	void* rsp;
	void* code_page;
	void* data_page;

	uint64 real_mode_gdt[3] = { 0x0, 0x000F9A000000FFFF, 0x000F92000000FFFF };
	uint16 real_gdtr_size = 3 * sizeof(uint64) - 1;
	uint32 real_gdtr_offset = 0x00000000;

	uint64 compat_mode_gdt[3] = { 0x0, 0x00CF9A000000FFFF, 0x00CF92000000FFFF };
	uint16 compat_gdtr_size = 3 * sizeof(uint64) - 1;
	uint64 compat_gdtr_offset = 0x00000000;

	uint64 long_mode_gdt[3] = { 0x0, 0x00209A0000000000, 0x0000920000000000 };
	uint16 long_gdtr_size = 3 * sizeof(uint64) - 1;
	uint64 long_gdtr_offset = 0x00000000;

	struct {
		uint16 size;
		uint64 offset;
	} __attribute__((packed)) idtr;

	ControlBlock* ap_ctb;
	Thread* idle_task;
} __attribute__((packed));