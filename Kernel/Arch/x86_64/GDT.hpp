#pragma once

#include <Arch/x86_64/CPU.hpp>
#include <LibGeneric/Utility.hpp>
#include <SystemTypes.hpp>

namespace arch::x86_64 {
	///  Reload the GDT using the provided GDTR pointer.
	///
	///	 This performs an lgdt using the provided GDTR, followed by a reload
	///  of all selectors to their kernel default values (0x8 for code-related
	///  selectors, and 0x10 for data-related selectors).
	__attribute__((naked, no_stack_protector)) inline void lgdt(void*) {
		asm volatile("  push %rbp					 \n"
		             "  mov %rbp, %rsp				 \n"

		             //  rdi - arg0 (pointer to gdtr)
		             "  mov %rax, %rdi				 \n"
		             "  lgdt [%rax]					 \n"
		             //  Load the kernel code selector and far return into a stub.
		             "  mov %rax, 8					 \n"
		             "  pushq %rax					 \n"
		             "  movabs %rax, offset .reload  \n"
		             "  pushq %rax					 \n"
		             "  retfq						 \n"
		             //  Reload all selectors to the default kernel selectors.
		             ".reload: 						 \n"
		             "	mov %ax, 0x10				 \n"
		             "	mov %ds, %ax				 \n"
		             "	mov %es, %ax				 \n"
		             "	mov %fs, %ax				 \n"
		             "	mov %gs, %ax				 \n"
		             "	mov %ss, %ax				 \n"

		             "  mov %rsp, %rbp				 \n"
		             "  pop %rbp					 \n"
		             "	ret							 \n");
	}

	///  Load task register selector.
	__attribute__((no_stack_protector)) inline void ltr(uint16 selector) {
		asm volatile("ltr %0\n" ::"r"(selector) : "memory");
	}

	///  Task state segment structure
	///
	///  The kernel does not utilize the I/O permission bitmap, but it does
	///  use the TSS for switching interrupt stacks during user-to-kernel
	///  interrupt transitions.
	struct TSS {
		uint32 _rsvd_0x0;
		void* rsp0;
		void* rsp1;
		void* rsp2;
		uint32 _rsvd_0x1c;
		uint32 _rsvd_0x20;
		void* ist1;
		void* ist2;
		void* ist3;
		void* ist4;
		void* ist5;
		void* ist6;
		void* ist7;
		uint32 _rsvd_0x5c;
		uint32 _rsvd_0x60;
		uint32 iopb { sizeof(TSS) };
		//  IOPB is currently not used by the kernel.
	} __attribute__((packed));
	static_assert(sizeof(TSS) == 104, "TSS struct must be 104 bytes");

	///  Create the high TSS entry for a given TSS struct.
	inline uint64 tss_to_high_descriptor(TSS& tss) {
		return (reinterpret_cast<uintptr_t>(&tss)) >> 32u;
	}

	///  Create the low TSS entry for a given TSS struct.
	inline uint64 tss_to_low_descriptor(TSS& tss) {
		const auto address = gen::bitcast<uint64>(&tss);
		const auto base_h = (address & 0xFF000000) << 32u;
		const auto base_l = (address & 0x00FFFFFF) << 16u;
		const auto limit = sizeof(tss) & 0xFFFF;
		return base_h | 0x0000890000000000 | base_l | limit;
	}

	///  Default selectors configured by the kernel.
	enum class Selector : uint16 {
		KernelCode = 0x8,
		KernelData = 0x10,
		UserNull = 0x18,
		UserData = 0x20,
		UserCode = 0x28,
		TSS = 0x30,
	};

	///  GDT descriptor struct - load it using lgdt
	struct GDTR {
		uint16 limit;
		void* base;

		constexpr GDTR(void* base_, uint16 limit_)
		    : limit(limit_)
		    , base(base_) {}
	} __attribute__((packed));

	///  Global descriptor table used by the kernel
	///
	///  Each CPU will instantiate its own GDT using this struct as a template. Ensure
	///  entries here are synchronized with the values of the `Selector` enum.
	struct GDT {
		uint64 entries[8] = {
			0x0,               //  Null
			0x00209A0000000000,//  Code0
			0x0000920000000000,//  Data0
			0x0,               //  Null
			0x0000F20000000000,//  Data3
			0x0020FA0000000000,//  Code3
			0x0,               //  TSS_lower, placeholder to be filled at runtime
			0x0,               //  TSS_higher, placeholder to be filled at runtime
		};
		TSS* tss;
		GDTR descriptor;

		constexpr GDT(TSS& tss_)
		    : tss(&tss_)
		    , descriptor(entries, sizeof(entries) - 1) {}

		inline void load() {
			//  Populate TSS entries at runtime. It is not allowed to reinterpret_cast
			//  within a constexpr context, so they cannot be generated at compile time.
			entries[6] = tss_to_low_descriptor(*tss);
			entries[7] = tss_to_high_descriptor(*tss);
			lgdt(&descriptor);
			ltr(static_cast<uint16>(Selector::TSS));
		}
	};
}

///  FIXME: Hacky way to avoid changing existing consumers of GDT.
///  Delete this in the future and adapt consumers instead.
using namespace arch::x86_64;