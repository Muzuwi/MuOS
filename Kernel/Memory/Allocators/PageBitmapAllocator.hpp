#pragma once

#include <string.h>
#include <SystemTypes.hpp>
#include <Structs/KOptional.hpp>
#include <Memory/Ptr.hpp>

//  Page-backed bitmap allocator for pages
class PageBitmapAllocator {
	PhysAddr m_base;
	PhysAddr m_alloc_pool_base;
	size_t m_bitmap_size;
	size_t m_region_size;
	bool m_deferred_initialization;

	static inline int64 divround(const int64 n, const int64 d) {
		return ((n < 0) ^ (d < 0)) ? ((n - d / 2) / d) : ((n + d / 2) / d);
	}

	static inline PhysAddr page_align(PhysAddr addr) {
		auto v = (uintptr_t)addr.get();
		return PhysAddr { (void*)(v & ~(0x1000 - 1)) };
	}

	inline bool bit_get(size_t idx) {
		auto index = idx / 8;
		auto offset = idx % 8;
		auto& byte = *(m_base.as<uint8>() + index);

		return byte & (1u << offset);
	}

	inline void bit_set(size_t idx, bool value) {
		auto index = idx / 8;
		auto offset = idx % 8;
		auto& byte = *(m_base.as<uint8>() + index);

		byte &= ~(1u << offset);
		byte |= (value ? 1 : 0) << offset;
	}

	inline size_t page_count() const {
		return (m_region_size - m_bitmap_size) / 0x1000;
	}

	inline PhysAddr idx_to_physical(size_t idx) const {
		return PhysAddr { m_alloc_pool_base + idx * 0x1000 };
	}

	inline size_t physical_to_idx(PhysAddr addr) const {
		auto idx = (addr - m_alloc_pool_base) / 0x1000;
		return idx;
	}

	//  Quick skip through full bytes
	KOptional<size_t> find_one();

	//  FIXME:  Implement
	KOptional<size_t> find_many(size_t);

	void mark_bits(size_t idx, size_t count, bool value);

	KOptional<PhysAddr> alloc_one();

	KOptional<PhysAddr> alloc_pow2(size_t order);

	void free_impl(PhysAddr base, size_t order);

	bool contains_address(PhysAddr addr);

	void initialize();

	friend class PMM;
public:
	PageBitmapAllocator(PhysAddr base, size_t region_size);

	KOptional<PhysAddr> allocate(size_t count_order = 0);

	void free(PhysAddr address, size_t order);

	bool deferred_initialization() const { return m_deferred_initialization; }
};