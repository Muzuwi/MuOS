#include <Memory/Allocators/BumpAllocator.hpp>

BumpAllocator::BumpAllocator(void* start, size_t size) noexcept
		: m_start(start), m_size(size), m_current(start) {

}

void* BumpAllocator::allocate(size_t size) {
	if(reinterpret_cast<void*>(reinterpret_cast<uintptr_t>(m_current) + size) >= end()) {
		return nullptr;
	}

	auto val = m_current;
	m_current = reinterpret_cast<void*>(reinterpret_cast<uintptr_t>(m_current) + size);
	return val;
}
