#include <Memory/Allocators/PageBitmapAllocator.hpp>
#include <Memory/Units.hpp>
#include <Debug/kpanic.hpp>

PageBitmapAllocator::PageBitmapAllocator(PhysAddr base, size_t region_size)
		: m_base(base), m_region_size(region_size) {
	auto region_pages = region_size / 0x1000;
	m_bitmap_size = divround(region_pages, 8);

	//  Verify if the bitmap will be accessible with the bootstrap identity mappings,
	//  as only 1GiB of lower physical is mapped. Defer initialization of the region
	//  if not.
	if((uintptr_t)m_base.get() > 1024 * Units::MiB ||
	   (uintptr_t)(m_base + m_bitmap_size).get() > 1024 * Units::MiB) {
		m_deferred_initialization = true;
	} else {
		m_deferred_initialization = false;
		initialize();
	}
}

KOptional<PhysAddr> PageBitmapAllocator::allocate(size_t count_order) {
	if(count_order == 0) {
		return alloc_one();
	} else {
		return alloc_pow2(count_order);
	}
}

void PageBitmapAllocator::free(PhysAddr address, size_t order) {
	free_impl(address, order);
}

void PageBitmapAllocator::initialize() {
	auto pool_start = m_base + m_bitmap_size;
	auto pool_aligned = pool_start + 0x1000;
	pool_aligned = PhysAddr { (void*)((uintptr_t)pool_aligned.get() & ~(0x1000u - 1u)) };

	m_alloc_pool_base = pool_aligned;

	assert(m_bitmap_size < m_region_size);
	memset(m_base.get_mapped(), 0x0, m_bitmap_size);
	m_deferred_initialization = false;
}

bool PageBitmapAllocator::contains_address(PhysAddr addr) {
	//  FIXME
	return addr >= m_alloc_pool_base;
}

void PageBitmapAllocator::free_impl(PhysAddr base, size_t order) {
	if(!contains_address(base)) {
		return;
	}

	auto idx = physical_to_idx(base);
	//  Mark bits taken by the allocation as unused
	mark_bits(idx, (1u << order), false);
}

KOptional<PhysAddr> PageBitmapAllocator::alloc_pow2(size_t order) {
	auto idx = find_many((1u << order));
	if(!idx.has_value()) {
		return KOptional<PhysAddr> {};
	}

	mark_bits(idx.unwrap(), (1u << order), true);
	return KOptional<PhysAddr> { idx_to_physical(idx.unwrap()) };
}

KOptional<PhysAddr> PageBitmapAllocator::alloc_one() {
	auto idx = find_one();
	if(!idx.has_value()) {
		return KOptional<PhysAddr> {};
	}

	mark_bits(idx.unwrap(), 1, true);
	return KOptional<PhysAddr> { idx_to_physical(idx.unwrap()) };
}

void PageBitmapAllocator::mark_bits(size_t idx, size_t count, bool value) {
	for(size_t i = idx; i < idx + count; ++i) {
		auto old = bit_get(i);

		if(old == value) {
			kpanic();
			continue;
		}

		bit_set(i, value);
	}
}

KOptional<size_t> PageBitmapAllocator::find_one() {
	auto base = m_base.as<uint8_t>();
	auto ptr = base;
	while(ptr - base < m_bitmap_size) {
		if(*ptr == 0xff) {
			ptr++;
			continue;
		}

		auto byte = *ptr;
		for(unsigned i = 0; i < 8; ++i) {
			if(byte & (1u << i)) {
				continue;
			}
			auto idx = (ptr - base) * 8 + i;
			return KOptional<size_t> { idx };
		}

		ASSERT_NOT_REACHED();
	}

	return KOptional<size_t> {};
}

KOptional<size_t> PageBitmapAllocator::find_many(size_t) {
	//  FIXME: Implement
	ASSERT_NOT_REACHED();
}
