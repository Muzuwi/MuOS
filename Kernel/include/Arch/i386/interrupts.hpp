#pragma once
#include <stdint.h>
#include <stddef.h>

#define PIC_LOG "[8259pic]: "
#define IDT_LOG "[idt]: "

#define IDT_INTS_COUNT 256
#define PIC_MASTER_CMD 0x0020
#define PIC_MASTER_DATA 0x0021
#define PIC_SLAVE_CMD 0x00A0
#define PIC_SLAVE_DATA 0x00A1

#define ICW1_ICW4	0x01		/* ICW4 (not) needed */
#define ICW1_SINGLE	0x02		/* Single (cascade) mode */
#define ICW1_INTERVAL4	0x04		/* Call address interval 4 (8) */
#define ICW1_LEVEL	0x08		/* Level triggered (edge) mode */
#define ICW1_INIT	0x10		/* Initialization - required! */
 
#define ICW4_8086	0x01		/* 8086/88 (MCS-80/85) mode */
#define ICW4_AUTO	0x02		/* Auto (normal) EOI */
#define ICW4_BUF_SLAVE	0x08		/* Buffered mode/slave */
#define ICW4_BUF_MASTER	0x0C		/* Buffered mode/master */
#define ICW4_SFNM	0x10		/* Special fully nested (not) */


struct IDT_Entry{
	uint16_t offset_lower;
	uint16_t selector;
	uint8_t zero;
	uint8_t type_attr;
	uint16_t offset_higher;
};

namespace IDT {
	extern IDT_Entry interrupts_table[IDT_INTS_COUNT];

	void init_PIC();
	void init_IDT();	
}