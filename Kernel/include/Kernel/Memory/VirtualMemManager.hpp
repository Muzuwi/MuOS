#pragma once
#include <Kernel/SystemTypes.hpp>

class VirtualMemManager final {
	VirtualMemManager();
public:
	static VirtualMemManager& get();
	void parse_multiboot_mmap(uintptr_t*);
};
