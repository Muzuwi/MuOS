#pragma once
#include <stdint.h>

constexpr unsigned kernelcdescr_offset = 1,
		kernelddescr_offset = 2,
		usercdescr_offset = 3,
		userddescr_offset = 4,
		tss_offset = 5;

constexpr unsigned user_CS = usercdescr_offset * 8,
		user_DS = userddescr_offset * 8,
		kernel_CS = kernelcdescr_offset * 8,
		kernel_DS = kernelddescr_offset * 8,
		TSS_sel = tss_offset * 8;

#define TSS_RSP0 (4)
#define TSS_IOPB (0x66)

namespace GDT {
	void init();
	void set_irq_stack(void*);

	constexpr unsigned get_user_CS() {
		return user_CS;
	}

	constexpr unsigned get_user_DS() {
		return user_DS;
	}

	constexpr unsigned get_kernel_CS() {
		return kernel_CS;
	}

	constexpr unsigned get_kernel_DS() {
		return kernel_DS;
	}

	void lgdt(void* ptr);
}
