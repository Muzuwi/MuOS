#pragma once
#include <Arch/x86_64/CPU.hpp>
#include <Arch/x86_64/GDT.hpp>
#include "SystemTypes.hpp"

namespace arch::mp {
	struct ExecutionEnvironment {
		ExecutionEnvironment* self_reference { this };//  0, store self reference for quick fetching
		Thread* thread {};                            //  8
		uint64 apic_id {};                            //  16
		uint64 _scratch {};                           //  24, only used in SysEntry to temporarily preserve user rsp
		TSS tss {};
		GDT gdt { tss };
	};

	static_assert(offsetof(ExecutionEnvironment, self_reference) == 0x0,
	              "self-reference of ExecutionEnvironment must be at offset 0");

	static_assert(offsetof(ExecutionEnvironment, thread) == 0x8,
	              "pointer to current Thread in ExecutionEnvironment must be at offset 0x8");

	static_assert(offsetof(ExecutionEnvironment, _scratch) == 0x18,
	              "scratch space in ExecutionEnvironment must be at offset 0x18");
}
