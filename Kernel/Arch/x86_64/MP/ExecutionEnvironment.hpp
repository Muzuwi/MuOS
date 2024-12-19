#pragma once
#include <Arch/x86_64/CPU.hpp>
#include <Arch/x86_64/GDT.hpp>
#include <Core/MP/MP.hpp>
#include "SystemTypes.hpp"

namespace arch::mp {
	struct ExecutionEnvironment {
		ExecutionEnvironment* self_reference; //  0, store self reference for quick fetching
		core::task::Task* task {};            //  8
		uint64 apic_id {};                    //  16
		uint64 _scratch {};                   //  24, only used in SysEntry to temporarily preserve user rsp
		core::mp::Environment* environment {};//  Higher-level kernel environment
		GDT gdt {};

		constexpr ExecutionEnvironment()
		    : self_reference(this) {}
	};

	static_assert(offsetof(ExecutionEnvironment, self_reference) == 0x0,
	              "self-reference of ExecutionEnvironment must be at offset 0");

	static_assert(offsetof(ExecutionEnvironment, task) == 0x8,
	              "pointer to current task in ExecutionEnvironment must be at offset 0x8");

	static_assert(offsetof(ExecutionEnvironment, _scratch) == 0x18,
	              "scratch space in ExecutionEnvironment must be at offset 0x18");

	ExecutionEnvironment* create_environment();
}

static inline arch::mp::ExecutionEnvironment* this_execution_environment() {
	auto read_env = []() -> void* {
		uint64 data;
		asm volatile("mov %0, %%gs:0\n" : "=r"(data) :);
		return (void*)data;
	};
	return static_cast<arch::mp::ExecutionEnvironment*>(read_env());
}
