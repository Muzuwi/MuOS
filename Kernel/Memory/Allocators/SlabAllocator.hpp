#pragma once

#include <SystemTypes.hpp>
#include <Memory/Allocators/VBitmap.hpp>

class SlabAllocator {
	VBitmap m_allocations {};
	void* m_pool_base {};
	size_t m_pool_size {};
	size_t m_object_size {};

	void* idx_to_virtual(size_t idx) const;

	size_t virtual_to_idx(void*) const;

	SlabAllocator(void* allocator_base, size_t pool_size, size_t object_size);

public:
	SlabAllocator() = default;

	SlabAllocator(SlabAllocator const&) = default;

	~SlabAllocator() = default;

	static KOptional<SlabAllocator> make(size_t pool_size, size_t object_size);

	void* allocate();

	void free(void*);

	bool contains_address(void*) const;

	size_t object_size() const { return m_object_size; }

	size_t objects_used() const { return m_allocations.used(); }

	size_t objects_free() const { return m_allocations.entries() - m_allocations.used(); };

	size_t pool_size() const { return m_pool_size; }

	void* pool_base() { return m_pool_base; }
};