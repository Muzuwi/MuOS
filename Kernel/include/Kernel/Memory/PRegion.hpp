#pragma once
#include <Kernel/Debug/kpanic.hpp>
#include <LibGeneric/BitMap.hpp>
#include <Kernel/Symbols.hpp>

class PRegion {
	void* m_start;
	void* m_end;
	gen::BitMap m_bitmap;

	~PRegion() {
		//  These should probably never get deallocated, if they did something went horribly wrong
		kpanic();
	}
public:
	PRegion(uint64_t base, uint64_t size)
	: m_start((void*)base), m_end((void*)(base + size)), m_bitmap(size / 4096) {
		//  Kernel physical symbols
		auto kernel_end = (uint64_t)TO_PHYS(&_ukernel_end);
		auto kernel_start = (uint64_t)TO_PHYS(&_ukernel_start);

		//  If our region intersects with kernel memory, set allocations so we don't give kernel memory away
		if((uint64_t)m_start < kernel_end) {
			if((uint64_t)m_end < kernel_start) return;

			uint64_t cur = kernel_start;
			if((uint64_t)m_start > kernel_end) cur = (uint64_t)m_start;

			while(cur < kernel_end && cur < (uint64_t)m_end) {
				m_bitmap.set((cur - (uint64_t)m_start) / 4096);
				cur += 4096;
			}
		}
	}

	void* alloc_page() {
		//  Find a free page in the bitmpa
		auto n = m_bitmap.find_seq_clear(1);

		//  No free pages left in this region
		if(n == m_bitmap.count())
			return nullptr;

		void* addr = (void*)((uint64_t)m_start + (4096 * n));
		//  Mark page as allocated
		m_bitmap.set(n);

		return addr;
	}

	void free_page(void* addr) {
		//  Not our page
		if(addr < m_start || addr >= m_end) return;

		unsigned n = ((uint64_t)addr - (uint64_t)m_start) / 4096;
		if(!m_bitmap[n])
			kerrorf("Tried freeing page from PRegion but the page was not allocated\n");

		m_bitmap.set(n, false);
	}

	bool has_address(void* addr) const {
		return addr >= m_start && addr < m_end;
	}

	void* addr() const {
		return m_start;
	}

	size_t size() const {
		return (uint64_t)m_end - (uint64_t)m_start + 1;
	}
};