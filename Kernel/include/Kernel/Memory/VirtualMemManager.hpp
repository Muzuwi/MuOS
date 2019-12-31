#pragma once
#include <Kernel/SystemTypes.hpp>

class VirtualMemManager final {
	VirtualMemManager();
public:
	static VirtualMemManager& get();
};
