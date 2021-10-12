#pragma once
#include <stdint.h>
#include <stddef.h>
#include <Kernel/KOptional.hpp>
#include <Kernel/Memory/PAllocation.hpp>
#include <Kernel/Memory/Ptr.hpp>

class PRegion;
class MultibootInfo;

namespace PMM {
	[[nodiscard]] KOptional<PAllocation> allocate(size_t count_order = 0);
	void free_allocation(const PAllocation& allocation);
	void handle_multiboot_memmap(PhysPtr<MultibootInfo>);
	void initialize_deferred_regions();
};
