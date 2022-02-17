#pragma once

#include <Memory/Allocators/PageBitmapAllocator.hpp>
#include <Memory/Ptr.hpp>

class PRegion {
	PhysAddr m_region_base;
	size_t m_region_size;
	//  FIXME:  Replace with buddy allocator when it's done
	PageBitmapAllocator m_allocator;
public:
	PRegion(PhysAddr base, size_t size)
	    : m_region_base(base)
	    , m_region_size(size)
	    , m_allocator(base, size) {}

	PhysAddr base() const { return m_region_base; }

	size_t size() const { return m_region_size; }

	PageBitmapAllocator& allocator() { return m_allocator; }

	bool contains(PhysAddr address) { return address >= m_region_base && address < m_region_base + m_region_size; }
};