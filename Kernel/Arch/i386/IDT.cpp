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
extern "C" void _kernel_syscall_entry();
extern "C" void* exception_entrypoint_table[32];
extern "C" void* irq_entrypoint_table[256 - 32];

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
	for(unsigned i = 0; i < 32; ++i)
		register_interrupt_gate(i, exception_entrypoint_table[i], type_trap, cs);

	//  Interrupts
	for(unsigned i = 32; i < 256; ++i)
		register_interrupt_gate(i, irq_entrypoint_table[i - 32], type_irq, cs);

	//  Legacy syscall gate
	const uint8_t type_syscall {0xF | (0b11 << 5) | (1 << 7)};
	register_interrupt_gate(0x80, _kernel_syscall_entry, type_syscall, GDT::get_kernel_CS());

	IDTR.limit = sizeof(interrupt_descr_table);
	IDTR.base = reinterpret_cast<uint64_t>(interrupt_descr_table);
	lidt(&IDTR);

	kdebugf("[IDT] registered interrupt handlers\n");
}
