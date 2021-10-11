#pragma once
#include <Kernel/SystemTypes.hpp>
#include <Arch/i386/GDT.hpp>

struct PtraceRegs {
	uint64_t r15;
	uint64_t r14;
	uint64_t r13;
	uint64_t r12;
	uint64_t r11;
	uint64_t r10;
	uint64_t r9;
	uint64_t r8;
	uint64_t rbp;
	uint64_t rdi;
	uint64_t rsi;
	uint64_t rdx;
	uint64_t rcx;
	uint64_t rbx;
	uint64_t rax;
	//  Error code during exceptions (or padding with qword[0], when the exception does not provide an error code)
	//  IRQ number during interrupt
	uint64_t origin;
	//  IRETQ frame
	uint64_t rip;
	uint64_t cs;
	uint64_t rflags;
	uint64_t rsp;
	uint64_t ss;

	static PtraceRegs user_default() {
		PtraceRegs regs {};
		regs.rflags = 0x200;
		regs.cs = GDT::get_user_CS() | 3;
		regs.ss = GDT::get_user_DS() | 3;
		return regs;
	}

	static PtraceRegs kernel_default() {
		PtraceRegs regs {};
		regs.rflags = 0x200;
		regs.cs = GDT::get_kernel_CS();
		regs.ss = GDT::get_kernel_DS();
		return regs;
	}
};

static_assert(sizeof(PtraceRegs) == 21 * sizeof(uint64), "PtraceRegs has invalid size");