#pragma once
#include <LibAllocator/Arena.hpp>
#include <stddef.h>
#include <stdint.h>

namespace liballoc {
	enum class ChunkState {
		Free,
		Allocated
	};

	struct Chunk {
		Chunk() = default;

		Chunk(size_t size)
		    : next(nullptr)
		    , size(size)
		    , state(ChunkState::Free) {}

		void* alloc_ptr() { return data; }

		void* alloc_end_ptr() { return reinterpret_cast<uint8_t*>(alloc_ptr()) + size; }

		Chunk* next;
		size_t size;
		ChunkState state;
		uint8_t data[];
	};

	class ChunkAllocator {
	public:
		ChunkAllocator() noexcept = default;
		ChunkAllocator(liballoc::Arena arena);

		void* allocate(size_t size);
		void free(void*);
	private:
		Chunk* m_chunk_list;
		size_t m_region_size;
		void* m_start;

		Chunk* find_free_chunk(size_t size);
		Chunk* find_chunk_with_address(void* addr);
		bool mark_chunk_allocated(Chunk&, size_t);
	};
}
