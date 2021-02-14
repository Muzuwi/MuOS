#pragma once
#include <Kernel/SystemTypes.hpp>
#include <Kernel/Memory/PhysBitmap.hpp>
#include <Kernel/Memory/Units.hpp>

class SlabAllocator {
	PhysBitmap m_allocation_bitmap;
	PhysAddr m_pool_base;
	void* m_virtual_start;
	size_t m_pool_order;
	size_t m_object_size;
	size_t m_object_count;

	void* idx_to_virtual(size_t idx) const;
	size_t virtual_to_idx(void*) const;
public:
	SlabAllocator(size_t object_size = 8, size_t alloc_pool_order = 3);
	void initialize(void* virtual_base);

	void* allocate();
	void free(void*);
	bool contains_address(void*) const;

	size_t object_size() const;
	size_t objects_used() const;
	size_t objects_free() const;
	size_t virtual_size() const;
};