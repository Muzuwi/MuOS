#include <Debug/kpanic.hpp>
#include <SystemTypes.hpp>
#include <Syscalls/Syscall.hpp>
#include <Process/Process.hpp>
#include <Arch/x86_64/CPU.hpp>
#include <Arch/x86_64/CPUID.hpp>
#include <Arch/x86_64/GDT.hpp>
#include <Arch/x86_64/PortIO.hpp>
#include <Debug/klogf.hpp>

void CPU::initialize_features() {
	uint32_t new_efer {0x500}; //  Long-Mode enable + Long-mode active

	klogf_static("[CPU] Support ");
	if(CPUID::has_NXE()) {
		klogf_static("NXE ");
		new_efer |= (1u << 11);
	}
	if(CPUID::has_SEP()) {
		klogf_static("SEP ");
		new_efer |= 1;
	}
	if(CPUID::has_LAPIC()) {
		klogf_static("LAPIC ");
	}
	klogf_static("\n");

	wrmsr(0xC0000080, (uint64_t)new_efer);

	//  Initialize SYSCALL MSRs
	//  STAR - kernel and user segment base
	wrmsr(0xC0000081, (uint64_t)GDT::get_kernel_CS() << 32ul | ((uint64_t)GDT::get_user_base_seg() | 3) << 48ul);
	//  Syscall entry
	wrmsr(0xC0000082, (uint64_t)_ukernel_syscall_entry);
	//  Flag mask - clear IF on syscall entry
	wrmsr(0xC0000084, 1u << 9u);
}

extern "C" void _switch_to_asm(Thread*, Thread*);

void CPU::switch_to(Thread* prev, Thread* next) {
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

uint64_t CPU::get_gs_base() {
	return rdmsr(0xC0000101);
}

uint64 CPU::cr2() {
	uint64 data = 0;
	asm volatile(
			"mov %0, %%cr2"
			:"=a"(data)
			::);
	return data;
}

uint64 CPU::cr3() {
	uint64 data = 0;
	asm volatile(
	"mov %0, %%cr3"
	:"=a"(data)
	::);
	return data;
}

extern "C" [[noreturn]] void _bootstrap_user(PtraceRegs* regs);

[[noreturn]] void CPU::jump_to_user(PtraceRegs* regs) {
	_bootstrap_user(regs);
}
