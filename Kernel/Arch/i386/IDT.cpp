#include <Arch/i386/IDT.hpp>
#include <Arch/i386/PortIO.hpp>
#include <Kernel/Debug/kdebugf.hpp>
#include <Arch/i386/GDT.hpp>

/*
	Actual Interrupt descriptor table in memory
*/
IDT_Entry IDT::interrupts_table[IDT_INTS_COUNT] = {};

//  FIXME:  Temp, will be moved
uint64_t IDTR = 0;


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
extern "C" int isr_except_divbyzero();
extern "C" int isr_except_dbg();
extern "C" int isr_except_nmi();
extern "C" int isr_except_break();
extern "C" int isr_except_overflow();
extern "C" int isr_except_bound();
extern "C" int isr_except_invalidop();
extern "C" int isr_except_nodevice();
extern "C" int isr_except_doublefault();
extern "C" int isr_except_invalidtss();
extern "C" int isr_except_invalidseg();
extern "C" int isr_except_segstackfault();
extern "C" int isr_except_gpf();
extern "C" int isr_except_pagefault();
extern "C" int isr_except_x87fpfault();
extern "C" int isr_except_aligncheck();
extern "C" int isr_except_machinecheck();
extern "C" int isr_except_simdfp();
extern "C" int isr_except_virtfault();
extern "C" int isr_except_security();
extern "C" void _kernel_syscall_entry();

inline void lidt(uint64_t *idtr) {
	asm volatile(
	            "cli\n"
	            "mov %%eax, %0\n"
	            "lidt [%%eax]\n"
	            "sti\t\n"
	            :
	            : ""((uintptr_t)idtr)
	            :
	            );
}



/*
	This function sets up the Interrupt Descriptor Table
	with entries for basic 15 interrupts and loads it
	into IDTR
*/
void IDT::init_IDT(){
	//  Interrupt handler entrypoint address
	uint32_t irq_addr;

	//  Exception handler entrypoint adress
	uint32_t except_addr; 

	#define TOSTRING(a) #a
	#define CAT(a, b) a##b
	#define SETIRQ(irqname, number) \
		irq_addr = (uint32_t)(irqname); \
		interrupts_table[number].zero = 0; \
		interrupts_table[number].offset_lower = irq_addr & 0xFFFF; \
		interrupts_table[number].offset_higher = (irq_addr & 0xFFFF0000) >> 16; \
		interrupts_table[number].selector = 0x08;	\
		interrupts_table[number].type_attr = 0x8e;

	#define SETEXCEPTION(excname, number) \
		except_addr = (uint32_t)(excname); \
		interrupts_table[number].zero = 0; \
		interrupts_table[number].offset_lower = except_addr & 0xFFFF; \
		interrupts_table[number].offset_higher = (except_addr & 0xFFFF0000) >> 16; \
		interrupts_table[number].selector = 0x08;	\
		interrupts_table[number].type_attr = 0x8f;

	SETEXCEPTION(isr_except_divbyzero, 0)
	SETEXCEPTION(isr_except_dbg, 1)
	SETEXCEPTION(isr_except_nmi, 2)
	SETEXCEPTION(isr_except_break, 3)
	SETEXCEPTION(isr_except_overflow, 4)
	SETEXCEPTION(isr_except_bound, 5)
	SETEXCEPTION(isr_except_invalidop, 6)
	SETEXCEPTION(isr_except_nodevice, 7)
	SETEXCEPTION(isr_except_doublefault, 8)
	//  9 legacy
	SETEXCEPTION(isr_except_invalidtss, 10)
	SETEXCEPTION(isr_except_invalidseg, 11)
	SETEXCEPTION(isr_except_segstackfault, 12)
	SETEXCEPTION(isr_except_gpf, 13)
	SETEXCEPTION(isr_except_pagefault, 14)
	//  15 reserved
	SETEXCEPTION(isr_except_x87fpfault, 16)
	SETEXCEPTION(isr_except_aligncheck, 17)
	SETEXCEPTION(isr_except_machinecheck, 18)
	SETEXCEPTION(isr_except_simdfp, 19)
	SETEXCEPTION(isr_except_virtfault, 20)
	//  21-29 reserved
	SETEXCEPTION(isr_except_security, 30)
	//  31 reserved



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

	//  Setup syscall handler
	interrupts_table[0x80].zero = 0;
	interrupts_table[0x80].offset_lower = (uint32_t)(_kernel_syscall_entry) & 0xFFFF;
	interrupts_table[0x80].offset_higher = ((uint32_t)(_kernel_syscall_entry) & 0xFFFF0000) >> 16;
	interrupts_table[0x80].selector = GDT::get_kernel_CS();
	interrupts_table[0x80].type_attr = 0xF | (0b11 << 5) | (1 << 7);


	uint16_t idt_size = sizeof(interrupts_table)*sizeof(IDT_Entry);
	IDTR |= idt_size;
	IDTR |= ((uint64_t)interrupts_table) << 16;

	lidt(&IDTR);
	kdebugf(IDT_LOG "IDT loaded\n");
}


/*
	Remaps the PIC to offset interrupts
	by 0x20 to avoid overlapping standard arch interrupts (?)
*/
extern "C" void remapPIC();
void IDT::init_PIC(){

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

	kdebugf(PIC_LOG "PIC initialization complete\n");
}
