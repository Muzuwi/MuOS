#include <string.h>
#include <Debug/kpanic.hpp>
#include <Memory/Allocators/SlabAllocator.hpp>
#include <Memory/PMM.hpp>
#include <Process/Process.hpp>
#include <Debug/klogf.hpp>

SlabAllocator::SlabAllocator(size_t object_size, size_t alloc_pool_order) {
	m_object_size = object_size;
	m_pool_order = alloc_pool_order;
	m_virtual_start = nullptr;
}

void SlabAllocator::initialize(void* virtual_base) {
	m_virtual_start = virtual_base;

	auto alloc = PMM::instance().allocate();
	if(!alloc.has_value()) {
		klogf_static("Failed allocating page for SlabAllocator bitmap!\n");
		kpanic();
	}
	m_allocation_bitmap = PhysBitmap { alloc.unwrap().base(), object_capacity() };

	auto pool = PMM::instance().allocate(m_pool_order);
	if(!pool.has_value()) {
		klogf_static("Failed allocating pool for SlabAllocator({})!\n", m_object_size);
		kpanic();
	}
	m_pool_base = pool.unwrap().base();
	memset(m_pool_base.get_mapped(), 0, 0x1000 << m_pool_order);

//	SMP::ctb().current_thread()->parent()->vmm()._map_pallocation(pool.unwrap(), m_virtual_start);
	Process::_kerneld_ref().vmm()._map_pallocation(pool.unwrap(), m_virtual_start);
}

void* SlabAllocator::allocate() {
	auto idx = m_allocation_bitmap.allocate();
	if(!idx.has_value()) {
		klogf_static("SlabAllocator: Failed allocating memory for object of size {}\n", m_object_size);
		return nullptr;
	}

	return idx_to_virtual(idx.unwrap());
}

void SlabAllocator::free(void* addr) {
	if(!contains_address(addr)) {
		return;
	}
	auto idx = virtual_to_idx(addr);
	m_allocation_bitmap.free(idx, 1);
}

bool SlabAllocator::contains_address(void* addr) const {
	return addr >= m_virtual_start &&
	       (uintptr_t)addr < (uintptr_t)m_virtual_start + virtual_size();
}

void* SlabAllocator::idx_to_virtual(size_t idx) const {
	return (void*)((uintptr_t)m_virtual_start + idx * m_object_size);
}

size_t SlabAllocator::virtual_to_idx(void* addr) const {
	return ((uintptr_t)addr - (uintptr_t)m_virtual_start) / m_object_size;
}

size_t SlabAllocator::object_size() const {
	return m_object_size;
}

size_t SlabAllocator::objects_used() const {
	return m_allocation_bitmap.used();
}

size_t SlabAllocator::objects_free() const {
	return m_allocation_bitmap.entries() - m_allocation_bitmap.used();
}

size_t SlabAllocator::virtual_size() const {
	return m_object_size * object_capacity();
}

size_t SlabAllocator::object_capacity() const {
	return (0x1000 << m_pool_order) / m_object_size;
}
