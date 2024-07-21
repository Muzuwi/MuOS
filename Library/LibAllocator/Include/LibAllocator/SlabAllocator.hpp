#pragma once
#include <LibAllocator/Arena.hpp>
#include <stddef.h>
#include <stdint.h>

namespace liballoc {
	class SlabAllocator {
	public:
		//  FIXME: The deafult constructor can be removed once KHeap is sane
		SlabAllocator()
		    : m_arena(nullptr, 0) {}
		SlabAllocator(liballoc::Arena arena, size_t object_size);

		void* allocate();
		void free(void*);

		[[nodiscard]] constexpr void* start() const { return m_arena.base; }

		[[nodiscard]] void* end() const { return m_arena.end(); }

		[[nodiscard]] constexpr size_t size() const { return m_arena.length; }

		[[nodiscard]] constexpr size_t object_size() const { return m_object_size; }

		[[nodiscard]] constexpr void* bitmap_start() const { return m_bitmap_start; }

		[[nodiscard]] constexpr size_t bitmap_size() const { return m_bitmap_size; }

		[[nodiscard]] constexpr void* pool_start() const { return m_pool_start; }

		[[nodiscard]] constexpr size_t pool_capacity() const { return m_pool_capacity; }

		[[nodiscard]] constexpr size_t pool_size() const { return m_pool_capacity * m_object_size; }

		[[nodiscard]] constexpr size_t overhead() const { return m_overhead; }
	private:
		Arena m_arena;
		size_t m_object_size {};

		void* m_bitmap_start;
		size_t m_bitmap_size;
		void* m_pool_start;
		size_t m_pool_capacity;

		size_t m_overhead;
	};
}
