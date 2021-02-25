#pragma once
#include <Kernel/Memory/VMapping.hpp>

class VMRegion {
	gen::SharedPtr<VMapping> m_mapping;
	void* m_start;
	size_t m_size;
public:
	VMRegion(gen::SharedPtr<VMapping> const& mapping, size_t size) noexcept
	: m_mapping(mapping), m_start(mapping->addr()), m_size(size) {}

	size_t size() const {
		return m_size;
	}

	bool mapped() const {
		return (bool)m_mapping;
	}

	void* start() const {
		return m_start;
	}

	VMapping& mapping() {
		return *m_mapping;
	}
	VMapping const& mapping() const{
		return *m_mapping;
	}

	bool contains(void* addr) const {
		return addr >= m_start && (uintptr_t)addr < (uintptr_t)m_start+m_size;
	}

};