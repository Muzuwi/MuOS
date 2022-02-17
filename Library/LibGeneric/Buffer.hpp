#pragma once
#include <stddef.h>
#include <stdint.h>

class Buffer {
	uint8_t* m_base;
	size_t m_size;
public:
	Buffer(size_t size) {
		m_base = new uint8_t[size];
		m_size = size;
	}
	Buffer(Buffer&& v) {
		m_base = v.m_base;
		m_size = v.m_size;
	}
	Buffer(const Buffer&) = delete;

	template<class T = uint8_t>
	T* raw_ptr() {
		return reinterpret_cast<T*>(m_base);
	}

	size_t size() const { return m_size; }

	bool contains(void* addr) {
		return ((uint64_t)addr >= (uint64_t)m_base) && ((uint64_t)addr < (uint64_t)m_base + m_size);
	}
};