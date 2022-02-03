#pragma once
#include <stdint.h>
#include <stddef.h>
#include <Structs/KOptional.hpp>
#include <Memory/PAllocation.hpp>
#include <Memory/Ptr.hpp>
#include <LibGeneric/StaticVector.hpp>
#include <Memory/PRegion.hpp>
#include <LibGeneric/Vector.hpp>
#include <Memory/kmalloc.hpp>

class MultibootInfo;

class PMM {
	static PMM s_instance;
	PMM() noexcept {}

	gen::Vector<PRegion, KMalloc::BootstrapAllocator> m_mem16_regions;  //  "Lowmem" - accessible from 16-bit
	gen::Vector<PRegion, KMalloc::BootstrapAllocator> m_normal_regions; //  Accessible from 32-bit
public:
	static PMM& instance() {
		return s_instance;
	}

	[[nodiscard]] KOptional<PAllocation> allocate(size_t count_order = 0);
	void free(PAllocation const&);

	[[nodiscard]] KOptional<PhysAddr> allocate_lowmem();
	void free_lowmem(PhysAddr);

	void init_regions(PhysPtr<MultibootInfo>);
	void init_deferred_allocators();
};
