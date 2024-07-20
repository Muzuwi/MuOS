#pragma once

#include <SystemTypes.hpp>

enum class ChunkState {
	Free,
	Allocated
};

struct Chunk {
	Chunk* m_next;
	size_t m_size;
	ChunkState m_state;

	Chunk() = default;

	Chunk(size_t size)
	    : m_next(nullptr)
	    , m_size(size)
	    , m_state(ChunkState::Free) {}

	void* alloc_ptr() { return reinterpret_cast<uint8*>(this) + sizeof(Chunk); }

	void* alloc_end_ptr() { return reinterpret_cast<uint8*>(alloc_ptr()) + m_size; }
};

class ChunkAllocator {
	Chunk* m_chunk_list;
	size_t m_region_size;
	void* m_start;

	Chunk* find_free_chunk(size_t size);

	Chunk* find_chunk_with_address(void* addr);

	bool mark_chunk_allocated(Chunk&, size_t);
public:
	ChunkAllocator() noexcept = default;

	ChunkAllocator(void* virtual_start, size_t size);

	void* allocate(size_t size);

	void free(void*);

	void dump_allocator();
};