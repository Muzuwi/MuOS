#include <Memory/Allocators/SlabAllocator.hpp>
#include <Memory/VMM.hpp>

SlabAllocator::SlabAllocator(void* allocator_base, size_t pool_size, size_t object_size)
    : m_allocations(allocator_base, pool_size / object_size)
    , m_pool_base(reinterpret_cast<uint8*>(allocator_base) + m_allocations.buffer_size())
    , m_pool_size(pool_size)
    , m_object_size(object_size) {}

KOptional<SlabAllocator> SlabAllocator::make(size_t pool_size, size_t object_size) {
	const auto objects = pool_size / object_size;
	const auto required_bitmap_size = objects / 8;
	const auto required_pool_size = object_size * objects;
	const auto total_allocator_size = required_bitmap_size + required_pool_size;

	auto* allocator_area = VMM::allocate_kernel_heap(total_allocator_size);
	if(!allocator_area) {
		return KOptional<SlabAllocator> {};
	}

	return KOptional<SlabAllocator> {
		SlabAllocator {allocator_area, pool_size, object_size}
	};
}

void* SlabAllocator::allocate() {
	auto idx = m_allocations.allocate();
	if(!idx.has_value()) {
		return nullptr;
	}

	return idx_to_virtual(idx.unwrap());
}

void SlabAllocator::free(void* addr) {
	if(!contains_address(addr)) {
		return;
	}
	auto idx = virtual_to_idx(addr);
	m_allocations.free(idx, 1);
}

bool SlabAllocator::contains_address(void* addr) const {
	return addr >= m_pool_base && (uintptr_t)addr < (uintptr_t)m_pool_base + m_pool_size;
}

void* SlabAllocator::idx_to_virtual(size_t idx) const {
	return (void*)((uintptr_t)m_pool_base + idx * m_object_size);
}

size_t SlabAllocator::virtual_to_idx(void* addr) const {
	return ((uintptr_t)addr - (uintptr_t)m_pool_base) / m_object_size;
}