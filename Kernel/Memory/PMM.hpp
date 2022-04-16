#pragma once
#include <LibGeneric/Spinlock.hpp>
#include <LibGeneric/StaticVector.hpp>
#include <Memory/Allocators/PageBitmapAllocator.hpp>
#include <Memory/Allocators/PAllocation.hpp>
#include <Memory/Allocators/PRegion.hpp>
#include <Memory/Ptr.hpp>
#include <stddef.h>
#include <stdint.h>
#include <Structs/KOptional.hpp>

class MultibootInfo;

class PMM {
	static PMM s_instance;

	gen::Spinlock m_pmm_lock {};
	PhysAddr m_physical_end {};
	gen::StaticVector<PRegion, 256> m_mem16_regions {}; //  "Lowmem" - accessible from 16-bit
	gen::StaticVector<PRegion, 256> m_normal_regions {};//  Other regions, including those accessible only in 64-bit

	void create_normal_region(void* start, size_t size);
	void create_lowmem_region(void* start, size_t size);
	constexpr PMM() noexcept = default;
public:
	static PMM& instance() { return s_instance; }

	[[nodiscard]] KOptional<PAllocation> allocate(size_t count_order = 0);
	void free(PAllocation const&);

	[[nodiscard]] KOptional<PhysAddr> allocate_lowmem();
	void free_lowmem(PhysAddr);

	void init_regions(PhysPtr<MultibootInfo>);
	void init_deferred_allocators();

	PhysAddr physical_end_addr();
};
