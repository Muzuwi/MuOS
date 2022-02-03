#pragma once
#include <SystemTypes.hpp>

/*  Enter - real mode, 16 bit
 *      - cli
 *      - enable A20
 *      - load real->protected GDT
 *      - CR0:PE=1, far jump and reload selectors
 *   protected mode -> long mode + paging:
 *      - load GDT provided by BSP
 *      - load CR3 provided by BSP
 *      - enable all features
 *
 */

//  Keep this in sync with the one in APBoot.asm
struct APBoostrap
{
	uint64 state_flag;
	uint64 cr3;
	uint64 rsp;
	uint64 fsbase;
	uint64 gsbase;
	uint64 kgsbase;

	uint64 real_mode_gdt[3] = {
			0x0,
			0x000F9A000000FFFF,
			0x000F92000000FFFF
	};
	uint16 real_gdtr_size   = 3 * sizeof(uint64) - 1;
	uint32 real_gdtr_offset = 0x00000000;

	uint64 compat_mode_gdt[3] = {
			0x0,
			0x00CF9A000000FFFF,
			0x00CF92000000FFFF
	};
	uint16 compat_gdtr_size   = 3 * sizeof(uint64) - 1;
	uint64 compat_gdtr_offset = 0x00000000;

	uint64 long_mode_gdt[3] = {
			0x0,
			0x00209A0000000000,
			0x0000920000000000
	};
	uint16 long_gdtr_size   = 3 * sizeof(uint64) - 1;
	uint64 long_gdtr_offset = 0x00000000;

	struct {
		uint16 size;
		uint64 offset;
	} idtr;
} __attribute__((packed));