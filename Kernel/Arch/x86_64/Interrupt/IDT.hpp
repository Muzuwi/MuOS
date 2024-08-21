#pragma once
#include <stddef.h>
#include <stdint.h>

#define IDT_INTS_COUNT  256

struct IDT_Entry {
	uint16_t offset_0;
	uint16_t selector;
	uint8_t _zero1;
	uint8_t type_attr;
	uint16_t offset_16;
	uint32_t offset_32;
	uint32_t _zero2;
} __attribute__((packed));
static_assert(sizeof(IDT_Entry) == 16);

namespace IDT {
	void init_ap();
	void init();
}