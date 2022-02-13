#pragma once

#include <SystemTypes.hpp>

class BumpAllocator {
	void* m_start;
	size_t m_size;
	void* m_current;
public:
	BumpAllocator(void* start, size_t size) noexcept;

	void* allocate(size_t size);

	void* start() const { return m_start; }

	void* end() const { return reinterpret_cast<void*>(reinterpret_cast<uintptr_t>(m_start) + m_size); }

	void* current() const { return m_current; }

	size_t size() const { return m_size; }
};
