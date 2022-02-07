#pragma once

#include <Memory/Ptr.hpp>

class PAllocation {
	PhysAddr m_allocation_base;
	size_t m_allocation_order;
public:
	PAllocation() noexcept
			: m_allocation_base(nullptr), m_allocation_order(0) {}

	PAllocation(PhysAddr base, size_t size) noexcept
			: m_allocation_base(base), m_allocation_order(size) {}

	PhysAddr base() const { return m_allocation_base; }

	PhysAddr end() const { return m_allocation_base + size(); }

	PhysAddr last() const { return end() - 1; }

	size_t order() const { return m_allocation_order; }

	size_t size() const { return (1u << m_allocation_order) * 0x1000; }
};