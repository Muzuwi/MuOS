#include <Arch/i386/IDT.hpp>
#include <Arch/i386/PortIO.hpp>
#include <Kernel/Debug/kdebugf.hpp>
#include <Arch/i386/GDT.hpp>

static IDT_Entry interrupt_descr_table[IDT_INTS_COUNT] = {};

static struct {
	uint16_t limit;
	uint64_t base;
} __attribute__((packed)) IDTR;

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

extern "C" void _exception_entry_0();
extern "C" void _exception_entry_1();
extern "C" void _exception_entry_2();
extern "C" void _exception_entry_3();
extern "C" void _exception_entry_4();
extern "C" void _exception_entry_5();
extern "C" void _exception_entry_6();
extern "C" void _exception_entry_7();
extern "C" void _exception_entry_8();
extern "C" void _exception_entry_9();
extern "C" void _exception_entry_10();
extern "C" void _exception_entry_11();
extern "C" void _exception_entry_12();
extern "C" void _exception_entry_13();
extern "C" void _exception_entry_14();
extern "C" void _exception_entry_15();
extern "C" void _exception_entry_16();
extern "C" void _exception_entry_17();
extern "C" void _exception_entry_18();
extern "C" void _exception_entry_19();
extern "C" void _exception_entry_20();
extern "C" void _exception_entry_21();
extern "C" void _exception_entry_22();
extern "C" void _exception_entry_23();
extern "C" void _exception_entry_24();
extern "C" void _exception_entry_25();
extern "C" void _exception_entry_26();
extern "C" void _exception_entry_27();
extern "C" void _exception_entry_28();
extern "C" void _exception_entry_29();
extern "C" void _exception_entry_30();
extern "C" void _exception_entry_31();

extern "C" void _kernel_syscall_entry();

inline void lidt(void* idtr) {
	asm volatile(
	            "mov %%rax, %0\n"
	            "lidt [%%rax]\n"
	            :
	            : ""((uintptr_t)idtr)
	            : "rax"
	            );
}

/*
	Remaps the PIC to offset interrupts
	by 0x20 to avoid overlapping standard arch interrupts
*/
static void remap_pic(){
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
}

template<typename T>
static void register_interrupt_gate(uint8_t irq_num, T gate, uint8_t type, uint16_t selector) {
	auto& entry = interrupt_descr_table[irq_num];
	auto gate_address = reinterpret_cast<uint64_t>(gate);
	entry.selector = selector;
	entry.type_attr = type;
	entry._zero1 = 0;
	entry._zero2 = 0;

	entry.offset_0 = gate_address & 0xFFFFu;
	entry.offset_16 = (gate_address & 0xFFFF0000u) >> 16u;
	entry.offset_32 = (gate_address >> 32u);
}

/*
	This function sets up the Interrupt Descriptor Table
	with entries for basic 15 interrupts and loads it
	into IDTR
*/
void IDT::init(){
	remap_pic();

	const uint8_t type_irq {0x8e};
	const uint8_t type_trap {0x8f};
	const auto cs = GDT::get_kernel_CS();

	//  Exceptions
	register_interrupt_gate(0, _exception_entry_0, type_trap, cs);
	register_interrupt_gate(1, _exception_entry_1, type_trap, cs);
	register_interrupt_gate(2, _exception_entry_2, type_trap, cs);
	register_interrupt_gate(3, _exception_entry_3, type_trap, cs);
	register_interrupt_gate(4, _exception_entry_4, type_trap, cs);
	register_interrupt_gate(5, _exception_entry_5, type_trap, cs);
	register_interrupt_gate(6, _exception_entry_6, type_trap, cs);
	register_interrupt_gate(7, _exception_entry_7, type_trap, cs);
	register_interrupt_gate(8, _exception_entry_8, type_trap, cs);
	register_interrupt_gate(9, _exception_entry_9, type_trap, cs);
	register_interrupt_gate(10, _exception_entry_10, type_trap, cs);
	register_interrupt_gate(11, _exception_entry_11, type_trap, cs);
	register_interrupt_gate(12, _exception_entry_12, type_trap, cs);
	register_interrupt_gate(13, _exception_entry_13, type_trap, cs);
	register_interrupt_gate(14, _exception_entry_14, type_trap, cs);
	register_interrupt_gate(15, _exception_entry_15, type_trap, cs);
	register_interrupt_gate(16, _exception_entry_16, type_trap, cs);
	register_interrupt_gate(17, _exception_entry_17, type_trap, cs);
	register_interrupt_gate(18, _exception_entry_18, type_trap, cs);
	register_interrupt_gate(19, _exception_entry_19, type_trap, cs);
	register_interrupt_gate(20, _exception_entry_20, type_trap, cs);
	register_interrupt_gate(21, _exception_entry_21, type_trap, cs);
	register_interrupt_gate(22, _exception_entry_22, type_trap, cs);
	register_interrupt_gate(23, _exception_entry_23, type_trap, cs);
	register_interrupt_gate(24, _exception_entry_24, type_trap, cs);
	register_interrupt_gate(25, _exception_entry_25, type_trap, cs);
	register_interrupt_gate(26, _exception_entry_26, type_trap, cs);
	register_interrupt_gate(27, _exception_entry_27, type_trap, cs);
	register_interrupt_gate(28, _exception_entry_28, type_trap, cs);
	register_interrupt_gate(29, _exception_entry_29, type_trap, cs);
	register_interrupt_gate(30, _exception_entry_30, type_trap, cs);
	register_interrupt_gate(31, _exception_entry_31, type_trap, cs);

	//  Interrupts
	register_interrupt_gate(32, irq0, type_irq, cs);
	register_interrupt_gate(33, irq1, type_irq, cs);
	register_interrupt_gate(34, irq2, type_irq, cs);
	register_interrupt_gate(35, irq3, type_irq, cs);
	register_interrupt_gate(36, irq4, type_irq, cs);
	register_interrupt_gate(37, irq5, type_irq, cs);
	register_interrupt_gate(38, irq6, type_irq, cs);
	register_interrupt_gate(39, irq7, type_irq, cs);
	register_interrupt_gate(40, irq8, type_irq, cs);
	register_interrupt_gate(41, irq9, type_irq, cs);
	register_interrupt_gate(42, irq10, type_irq, cs);
	register_interrupt_gate(43, irq11, type_irq, cs);
	register_interrupt_gate(44, irq12, type_irq, cs);
	register_interrupt_gate(45, irq13, type_irq, cs);
	register_interrupt_gate(46, irq14, type_irq, cs);

	register_interrupt_gate(47, irq15, type_irq, cs);

	//  Legacy syscall gate
	const uint8_t type_syscall {0xF | (0b11 << 5) | (1 << 7)};
	register_interrupt_gate(0x80, _kernel_syscall_entry, type_syscall, GDT::get_kernel_CS());

	IDTR.limit = sizeof(interrupt_descr_table);
	IDTR.base = reinterpret_cast<uint64_t>(interrupt_descr_table);
	lidt(&IDTR);

	kdebugf("[IDT] registered interrupt handlers\n");
}
