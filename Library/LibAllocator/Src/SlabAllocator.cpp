#include <LibAllocator/Arena.hpp>
#include <LibAllocator/Bitmap.hpp>
#include <LibAllocator/SlabAllocator.hpp>

liballoc::SlabAllocator::SlabAllocator(liballoc::Arena arena, size_t object_size)
    : m_arena(arena)
    , m_object_size(object_size) {
	//  The bitmap will always start at byte 0 of the arena
	m_bitmap_start = m_arena.base;

	//  First, calculate the pool capacity as if the arena was used
	//  eclusively by it. This is of course not the case, as we have
	//  to fit in the bitmap as well, but it gives us an upper bound
	//  to use as the initial size.
	m_pool_capacity = m_arena.length / m_object_size;
	const auto initial_bitmap_size = (m_pool_capacity + 7) / 8;
	m_pool_start = static_cast<uint8_t*>(m_arena.base) + initial_bitmap_size;
	m_bitmap_size = initial_bitmap_size;

	//  Ensure the pool start address is always aligned to the object size
	const size_t align_mask = m_object_size - 1;
	const size_t pool_start_aligned = (reinterpret_cast<size_t>(m_pool_start) + align_mask) & ~align_mask;
	m_pool_start = reinterpret_cast<void*>(pool_start_aligned);

	//  Now, recalculate the actual amount of space we have for
	//  pool objects. Pool start address is already aligned and should
	//  not be changed.
	const auto pool_space_left = reinterpret_cast<uint8_t*>(m_arena.end()) - reinterpret_cast<uint8_t*>(m_pool_start);
	m_pool_capacity = pool_space_left / m_object_size;
	//  Update the bitmap to accomodate for the new pool capacity
	m_bitmap_size = (m_pool_capacity + 7) / 8;

	//  Track how many bytes of the arena we're losing
	m_overhead = m_arena.length - m_pool_capacity * m_object_size - m_bitmap_size;
}

void* liballoc::SlabAllocator::allocate() {
	size_t idx = 0;
	if(!liballoc::bitmap_find_one(m_bitmap_start, m_bitmap_size, idx)) {
		return nullptr;
	}
	liballoc::bitmap_set(m_bitmap_start, m_bitmap_size, idx, true);
	return (void*)(reinterpret_cast<uintptr_t>(m_pool_start) + idx * m_object_size);
}

void liballoc::SlabAllocator::free(void* addr) {
	auto idx = ((uintptr_t)addr - (uintptr_t)m_pool_start) / m_object_size;
	liballoc::bitmap_set(m_bitmap_start, m_bitmap_size, idx, false);
}
