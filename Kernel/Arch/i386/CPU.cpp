#include <Kernel/Debug/kdebugf.hpp>
#include <Kernel/Debug/kpanic.hpp>
#include <Kernel/SystemTypes.hpp>
#include <Kernel/Syscalls/Syscall.hpp>
#include <Kernel/Process/Process.hpp>
#include <Arch/i386/CPU.hpp>
#include <Arch/i386/CPUID.hpp>
#include <Arch/i386/GDT.hpp>
#include <Arch/i386/PortIO.hpp>

void CPU::initialize_features() {
	uint32_t new_efer {0x500}; //  Long-Mode enable + Long-mode active

	kdebugf("[CPU] Support ");
	if(CPUID::has_NXE()) {
		kdebugf("NXE ");
		new_efer |= (1u << 11);
	}
	if(CPUID::has_SEP()) {
		kdebugf("SEP ");
		new_efer |= 1;
	}
	if(CPUID::has_LAPIC()) {
		kdebugf("LAPIC ");
	}
	kdebugf("\n");

	wrmsr(0xC0000080, (uint64_t)new_efer);

	//  Initialize SYSCALL MSRs
	//  STAR - kernel and user segment base
	wrmsr(0xC0000081, (uint64_t)GDT::get_kernel_CS() << 32ul | ((uint64_t)GDT::get_user_base_seg() | 3) << 48ul);
	//  Syscall entry
	wrmsr(0xC0000082, (uint64_t)_ukernel_syscall_entry);
	//  Flag mask - clear IF on syscall entry
	wrmsr(0xC0000084, 1u << 9u);
}

extern "C" void _switch_to_asm(Process*, Process*);

#define S(a) (uintptr_t)a>>32u, (uintptr_t)a&0xffffffffu

void CPU::switch_to(Process* prev, Process* next) {
//	kdebugf("Switch from: %x%x [pid=%i], to %x%x [pid=%i]\n", (uintptr_t)prev>>32u, (uintptr_t)prev&0xffffffffu, prev->pid(), (uintptr_t)next>>32u, (uintptr_t)next&0xffffffffu, next->pid());
//	kdebugf("Nextframe=%x%x\n", S(next->frame()));
//	kdebugf("PML4=%x%x\n", S(next->pml4().get()));
	_switch_to_asm(prev, next);
}

void CPU::irq_disable() {
	asm volatile("cli");
}

void CPU::irq_enable() {
	asm volatile("sti");
}

void CPU::set_kernel_gs_base(void* p) {
	wrmsr(0xC0000102, (uint64_t)p);
}

uint64_t CPU::get_kernel_gs_base() {
	return rdmsr(0xC0000102);
}

void CPU::set_gs_base(void* p) {
	wrmsr(0xC0000101, (uint64_t)p);
}
