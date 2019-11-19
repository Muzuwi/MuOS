#pragma once
#include <kernel/SystemTypes.hpp>

class VirtualMemManager final {
	VirtualMemManager();
public:
	static VirtualMemManager& get();

	mem_range_t bootstrap(uint64_t size);
};