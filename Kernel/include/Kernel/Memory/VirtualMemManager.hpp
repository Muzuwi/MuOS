#pragma once
#include <Kernel/SystemTypes.hpp>
#include <Arch/i386/PageDirectory.hpp>
#include <Kernel/Symbols.hpp>

class VMM final {
	VMM() {}
	static PageDirectory* s_kernel_directory;
public:
	static PageDirectory* get_directory();
	static VMM& get();
	static void init();

	void map(uintptr_t*, uintptr_t*);
	void unmap(uintptr_t*);
};
