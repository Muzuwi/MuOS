#include <arch/i386/interrupts.hpp>
#include <arch/i386/portio.hpp>
#include <arch/i386/vga.h>
#include <stdio.h>

/*
	Actual Interrupt descriptor table in memory
*/
IDT_Entry IDT::interrupts_table[IDT_INTS_COUNT] = {};


/*
	IRQ handler wrappers
*/
extern "C" int irq0();
extern "C" int irq1();
extern "C" int irq2();
extern "C" int irq3();
extern "C" int irq4();
extern "C" int irq5();
extern "C" int irq6();
extern "C" int irq7();
extern "C" int irq8();
extern "C" int irq9();
extern "C" int irq10();
extern "C" int irq11();
extern "C" int irq12();
extern "C" int irq13();
extern "C" int irq14();
extern "C" int irq15();
extern "C" void loadIDT(uint32_t);

/*
	This function sets up the Interrupt Descriptor Table
	with entries for basic 15 interrupts and loads it
	into IDTR
*/
void IDT::init_IDT(){
	uint32_t irq0addr,irq1addr,irq2addr,irq3addr,
				  irq4addr,irq5addr,irq6addr,irq7addr,
				  irq8addr,irq9addr,irq10addr,irq11addr,
				  irq12addr,irq13addr,irq14addr,irq15addr;



	#define TOSTRING(a) #a
	#define CAT(a, b) a##b
	#define SETIRQ(irqname, number) \
		CAT(irqname, addr) = (uint32_t)(irqname); \
		interrupts_table[number].zero = 0; \
		interrupts_table[number].offset_lower = CAT(irqname, addr) & 0xFFFF; \
		interrupts_table[number].offset_higher = (CAT(irqname, addr) & 0xFFFF0000) >> 16; \
		interrupts_table[number].selector = 0x08;	\
		interrupts_table[number].type_attr = 0x8e;

	SETIRQ(irq0, 32)
	SETIRQ(irq1, 33)
	SETIRQ(irq2, 34)
	SETIRQ(irq3, 35)
	SETIRQ(irq4, 36)
	SETIRQ(irq5, 37)
	SETIRQ(irq6, 38)
	SETIRQ(irq7, 39)
	SETIRQ(irq8, 40)
	SETIRQ(irq9, 41)
	SETIRQ(irq10, 42)
	SETIRQ(irq11, 43)
	SETIRQ(irq12, 44)
	SETIRQ(irq13, 45)
	SETIRQ(irq14, 46)
	SETIRQ(irq15, 47)

	uint32_t idt_addr = (uint32_t)interrupts_table;
	uint64_t idtr = 0;

	uint16_t idt_size = sizeof(interrupts_table)*sizeof(IDT_Entry);
	idtr |= idt_size;
	idtr |= ((uint64_t)idt_addr) << 16;

	printf(IDT_LOG "Loading IDT table \n");
	loadIDT((uint32_t)&idtr);
	printf(IDT_LOG "IDT loaded\n");
}


/*
	Remaps the PIC to offset interrupts
	by 0x20 to avoid overlapping standard arch interrupts (?)
*/
extern "C" void remapPIC();
void IDT::init_PIC(){
	printf(PIC_LOG "Initializing PIC\n");

	out(PIC_MASTER_CMD, 0x11);
	out(PIC_SLAVE_CMD, 0x11);

	out(PIC_MASTER_DATA, 0x20);
	out(PIC_SLAVE_DATA, 40);

	out(PIC_MASTER_DATA, 4);
	out(PIC_SLAVE_DATA, 2);

	out(PIC_MASTER_DATA, ICW4_8086);
	out(PIC_SLAVE_DATA, ICW4_8086);

	out(PIC_MASTER_DATA, 0);
	out(PIC_SLAVE_DATA, 0);

	printf(PIC_LOG "PIC initialization complete\n");
}