#pragma once
#include <stdint.h>
#include <stddef.h>

#define IDT_INTS_COUNT 256
#define PIC_MASTER_CMD 0x0020
#define PIC_MASTER_DATA 0x0021
#define PIC_SLAVE_CMD 0x00A0
#define PIC_SLAVE_DATA 0x00A1

#define ICW4_8086	0x01		/* 8086/88 (MCS-80/85) mode */

struct IDT_Entry{
	uint16_t offset_0;
	uint16_t selector;
	uint8_t _zero1;
	uint8_t type_attr;
	uint16_t offset_16;
	uint32_t offset_32;
	uint32_t _zero2;
} __attribute__((packed));

namespace IDT {
	void init();
}