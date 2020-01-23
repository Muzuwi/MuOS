#pragma once
#include <Kernel/SystemTypes.hpp>
#include <Arch/i386/PageDirectory.hpp>

/*
 *	Start of kernel virtual address space
 */
extern uint32_t _ukernel_virtual_offset;

/*
 *	Convert between physical-virtual addresses
 */
#define TO_VIRT(a) ((uintptr_t)a + (uintptr_t)&_ukernel_virtual_offset)


class VMM final {
	VMM() {}
	static PageDirectory* s_kernel_directory;
public:
	static PageDirectory* get_directory();
	static VMM& get();
	static void init();

	void parse_multiboot_mmap(uintptr_t*);
};
