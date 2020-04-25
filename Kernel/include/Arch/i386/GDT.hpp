#pragma once
#include <stdint.h>

constexpr unsigned kernelcdescr_offset = 1,
		kernelddescr_offset = 2,
		usercdescr_offset = 3,
		userddescr_offset = 4,
		tss_offset = 5;

constexpr unsigned user_CS = usercdescr_offset * 8,
		user_DS = userddescr_offset * 8,
		kernel_CS = kernelcdescr_offset * 8,
		kernel_DS = kernelddescr_offset * 8,
		TSS_sel = tss_offset * 8;


#define TSS_ESP0 1
#define TSS_SS0  2
#define TSS_IOPB 25

#define SEGMENT_DESCTYPE(x) ((x) << 0x04)
#define SEGMENT_PRESENT(x) ((x) << 0x07)
#define SEGMENT_SAVAIL(x) ((x) << 0x0C) 
#define SEGMENT_GRAN(x) ((x) << 0x0F) //  0 - 1B blocks, 1 - 4 KiB blocks 
#define SEGMENT_SIZE(x) ((x) << 0x0E)		 //  0 - 16bit protected mode, 1 - 32bit protected mode
#define SEGMENT_LONG(x) ((x) << 0x0D)
#define SEGMENT_PRIV(x) (((x) & 0x3) << 0x5)

#define SEG_DATA_R        0x00 // Read-Only
#define SEG_DATA_RA       0x01 // Read-Only, accessed
#define SEG_DATA_RW      0x02 // Read/Write
#define SEG_DATA_RWA     0x03 // Read/Write, accessed
#define SEG_DATA_REXPD    0x04 // Read-Only, expand-down
#define SEG_DATA_REXPDA   0x05 // Read-Only, expand-down, accessed
#define SEG_DATA_RWEXPD  0x06 // Read/Write, expand-down
#define SEG_DATA_RWEXPDA 0x07 // Read/Write, expand-down, accessed
#define SEG_CODE_X        0x08 // Execute-Only
#define SEG_CODE_XA       0x09 // Execute-Only, accessed
#define SEG_CODE_XR      0x0A // Execute/Read
#define SEG_CODE_XRA     0x0B // Execute/Read, accessed
#define SEG_CODE_XC       0x0C // Execute-Only, conforming
#define SEG_CODE_XCA      0x0D // Execute-Only, conforming, accessed
#define SEG_CODE_XRC     0x0E // Execute/Read, conforming
#define SEG_CODE_XRCA    0x0F // Execute/Read, conforming, accessed


#define GDT_LOG "[gdt]: "

struct gdt_entry_t {
	uint32_t base;
	uint32_t limit;
	uint8_t access;
	uint8_t flags;
} __attribute__((packed));

namespace GDT {
	extern uint64_t descriptor_table[16];
	void init_GDT();
	void gdtentry_from_struct(uint16_t*, gdt_entry_t);
	uint64_t create_descriptor(uint32_t, uint32_t, uint16_t);

	constexpr unsigned get_user_CS() {
		return user_CS;
	}

	constexpr unsigned get_user_DS() {
		return user_DS;
	}

	constexpr unsigned get_kernel_CS() {
		return kernel_CS;
	}

	constexpr unsigned get_kernel_DS() {
		return kernel_DS;
	}
}
