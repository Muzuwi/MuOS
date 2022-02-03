#pragma once
#include <string.h>
#include <Kernel/KOptional.hpp>
#include <Kernel/Memory/Ptr.hpp>
#include <Kernel/Memory/Units.hpp>
#include <Debug/kpanic.hpp>

//  Page-backed bitmap allocator for pages
class PageBitmapAllocator {
	PhysAddr m_base;
	PhysAddr m_alloc_pool_base;
	size_t m_bitmap_size;
	size_t m_region_size;
	bool m_deferred_initialization;

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
	KOptional<size_t> find_one() {
		auto base = m_base.as<uint8_t>();
		auto ptr = base;
		while (ptr - base < m_bitmap_size) {
			if(*ptr == 0xff) {
				ptr++;
				continue;
			}

			auto byte = *ptr;
			for(unsigned i = 0; i < 8; ++i) {
				if(byte & (1u << i))
					continue;
				auto idx = (ptr-base)*8 + i;
				return KOptional<size_t>{idx};
			}

			ASSERT_NOT_REACHED();
		}

		return KOptional<size_t>{};
	}

	//  FIXME:  Implement
	KOptional<size_t> find_many(size_t) {
		ASSERT_NOT_REACHED();

//		size_t start = 0;
//		size_t current = 0;
//		while(current < page_count() - 1) {
//			if(bit_get(current)) {
//				start = current;
//			}
//			current++;
//		}

		return KOptional<size_t>{};
	}


	void mark_bits(size_t idx, size_t count, bool value) {

		for(size_t i = idx; i < idx + count; ++i) {
			auto old = bit_get(i);

			if(old == value) {
				kpanic();
				continue;
			}

			bit_set(i, value);
		}
	}


	PhysAddr idx_to_physical(size_t idx) {
		return PhysAddr {m_alloc_pool_base + idx*0x1000};
	}

	size_t physical_to_idx(PhysAddr addr) {
		auto idx = (addr - m_alloc_pool_base) / 0x1000;
		return idx;
	}

	KOptional<PhysAddr> alloc_one() {
		auto idx = find_one();
		if(!idx.has_value())
			return KOptional<PhysAddr> {};

		mark_bits(idx.unwrap(), 1, true);
		return KOptional<PhysAddr> {idx_to_physical(idx.unwrap())};
	}

	KOptional<PhysAddr> alloc_pow2(size_t order) {
		auto idx = find_many((1u << order));
		if(!idx.has_value())
			return KOptional<PhysAddr> {};

		mark_bits(idx.unwrap(), (1u << order), true);
		return KOptional<PhysAddr> {idx_to_physical(idx.unwrap())};
	}

	void _free(PhysAddr base, size_t order) {
		if(!contains_address(base))
			return;

		auto idx = physical_to_idx(base);
		//  Mark bits taken by the allocation as unused
		mark_bits(idx, (1u << order), false);
	}

	bool contains_address(PhysAddr addr) {
		//  FIXME
		return addr >= m_alloc_pool_base;
	}

	void initialize() {
		auto pool_start = m_base + m_bitmap_size;
		auto pool_aligned = pool_start + 0x1000;
		pool_aligned = PhysAddr{(void*)((uintptr_t)pool_aligned.get() & ~(0x1000u - 1u))};

		m_alloc_pool_base = pool_aligned;

//		kdebugf("PageBitmapAlloc: pool base %x%x, bitmap size %i, region size %i\n", (uint32_t)((uintptr_t)m_alloc_pool_base.get() >> 32u), (uint32_t)((uintptr_t)m_alloc_pool_base.get() & 0xFFFFFFFFu), m_bitmap_size, m_region_size);
//		kdebugf("PageBitmapAlloc: %i pages reserved\n", divround(m_bitmap_size, 0x1000));

		assert(m_bitmap_size < m_region_size);
		memset(m_base.get_mapped(), 0x0, m_bitmap_size);
		m_deferred_initialization = false;
	}

	friend class PMM;
public:
	PageBitmapAllocator(PhysAddr base, size_t region_size)
	: m_base(base), m_region_size(region_size) {
		auto region_pages = region_size / 0x1000;
		m_bitmap_size = divround(region_pages, 8);

		//  Verify if the bitmap will be accessible with the bootstrap identity mappings,
		//  as only 1GiB of lower physical is mapped. Defer initialization of the region
		//  if not.
		if((uintptr_t)m_base.get() > 1024 * Units::MiB || (uintptr_t)(m_base+m_bitmap_size).get() > 1024 * Units::MiB) {
			m_deferred_initialization = true;
		} else {
			m_deferred_initialization = false;
			initialize();
		}
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

	bool deferred_initialization() {
		return m_deferred_initialization;
	}
};