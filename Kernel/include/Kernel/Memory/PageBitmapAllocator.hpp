#pragma once
#include <string.h>
#include <Kernel/KOptional.hpp>
#include <Kernel/Memory/Ptr.hpp>

//  Page-backed bitmap allocator for pages
class PageBitmapAllocator {
	PhysAddr m_base;
	PhysAddr m_alloc_pool_base;
	size_t m_bitmap_size;
	size_t m_region_size;

	static inline int64_t divround(const int64_t n, const int64_t d) {
		return ((n < 0) ^ (d < 0)) ? ((n - d/2)/d) : ((n + d/2)/d);
	}

	static inline PhysAddr page_align(PhysAddr addr) {
		auto v = (uintptr_t)addr.get();
		return PhysAddr{(void*)(v & ~(0x1000 - 1))};
	}

	inline bool bit_get(size_t idx) {
		auto index = idx / 8;
		auto offset = idx % 8;
		auto& byte = *(m_base.as<uint8_t>() + index);

		return byte & (1u << offset);
	}

	inline void bit_set(size_t idx, bool value) {
		auto index = idx / 8;
		auto offset = idx % 8;
		auto& byte = *(m_base.as<uint8_t>() + index);

		byte &= ~(1u << offset);
		byte |= (value?1:0) << offset;
	}

	inline size_t page_count() const {
		return (m_region_size-m_bitmap_size) / 0x1000;
	}


	//  Quick skip through full bytes
	size_t find_one() {
		auto base = m_base.as<uint8_t>();
		auto ptr = base;
		while (ptr - base < m_bitmap_size) {
			if(*ptr == 0xff) continue;

			auto byte = *ptr;
			for(unsigned i = 0; i < 8; ++i) {
				if(byte & (1u << i))
					continue;
				auto idx = (ptr-base)*8 + i;
				return 1 + idx;
			}

			ptr++;
		}

		return 0;
	}

	//  FIXME:  Implement
	size_t find_many(size_t) {
		ASSERT_NOT_REACHED();

//		size_t start = 0;
//		size_t current = 0;
//		while(current < page_count() - 1) {
//			if(bit_get(current)) {
//				start = current;
//			}
//			current++;
//		}

		return 0;
	}


	void mark_bits(size_t idx, size_t count, bool value) {
		if(idx == 0)
			return;

		for(size_t i = idx; i < idx + count; ++i) {
			if(bit_get(i)) {
				kerrorf("PageBitmapAllocator: Corrupted state or double free for idx=%i\n", i);
			}
			bit_set(i, value);
		}
	}


	PhysAddr idx_to_physical(size_t idx) {
		if(idx == 0)
			return {};
		return PhysAddr {m_alloc_pool_base + idx*0x1000};
	}

	size_t physical_to_idx(PhysAddr addr) {
		auto aligned = page_align(addr);
		if(!contains_address(aligned))
			return 0;

		auto idx = (aligned - m_alloc_pool_base) / 0x1000;
		return 1 + idx;
	}


	KOptional<PhysAddr> alloc_one() {
		auto idx = find_one();
		if(idx == 0)
			return KOptional<PhysAddr> {};

		mark_bits(idx, 1, true);
		return KOptional<PhysAddr> {idx_to_physical(idx)};
	}

	KOptional<PhysAddr> alloc_pow2(size_t order) {
		auto idx = find_many((1u << order));
		if(idx == 0)
			return KOptional<PhysAddr> {};

		mark_bits(idx, (1u << order), true);
		return KOptional<PhysAddr> {idx_to_physical(idx)};
	}

	void _free(PhysAddr base, size_t order) {
		auto idx = physical_to_idx(base);
		if(idx == 0)
			return;

		//  Mark bits taken by the allocation as unused
		mark_bits(idx, (1u << order), false);
	}

	bool contains_address(PhysAddr addr) {
		//  FIXME
		return addr >= m_alloc_pool_base;
	}
public:
	PageBitmapAllocator(PhysAddr base, size_t region_size)
	: m_base(base), m_region_size(region_size) {
		auto region_pages = region_size / 0x1000;
		m_bitmap_size = divround(region_pages, 8);

		auto pool_start = base + m_bitmap_size;
		auto pool_aligned = pool_start + 0x1000;
		pool_aligned = PhysAddr{(void*)((uintptr_t)pool_aligned.get() & ~(0x1000u - 1u))};

		m_alloc_pool_base = pool_aligned;

//		kdebugf("PageBitmapAlloc: pool base %x%x, bitmap size %i, region size %i\n", (uint32_t)((uintptr_t)m_alloc_pool_base.get() >> 32u), (uint32_t)((uintptr_t)m_alloc_pool_base.get() & 0xFFFFFFFFu), m_bitmap_size, m_region_size);
//		kdebugf("PageBitmapAlloc: %i pages reserved\n", divround(m_bitmap_size, 0x1000));

		assert(m_bitmap_size < region_size);
		memset(m_base.get_mapped(), 0x0, m_bitmap_size);
	}

	KOptional<PhysAddr> allocate(size_t count_order = 0) {
		if(count_order == 0)
			return alloc_one();
		else
			return alloc_pow2(count_order);
	}

	void free(PhysAddr address, size_t order) {
		_free(address, order);
	}
};