#include <string.h>
#include <Memory/Allocators/PhysBitmap.hpp>
#include <Debug/kpanic.hpp>

PhysBitmap::PhysBitmap()
		: m_base(nullptr), m_entries(0), m_used(0) {

}

PhysBitmap::PhysBitmap(PhysAddr physical, size_t entries)
		: m_base(physical), m_entries(entries), m_used(0) {
	memset(physical.get_mapped(), 0, divround(m_entries, 8));
}

KOptional<size_t> PhysBitmap::allocate_impl(size_t count) {
	auto ret = (count == 1) ? find_one()
	                        : find_many(count);
	if(!ret.has_value()) {
		return KOptional<size_t> {};
	}

	mark_bits(ret.unwrap(), count, true);
	return ret;
}

void PhysBitmap::free_impl(size_t idx, size_t count) {
	//  Mark bits taken by the allocation as unused
	mark_bits(idx, count, false);
}

void PhysBitmap::mark_bits(size_t idx, size_t count, bool value) {
	for(size_t i = idx; i < idx + count; ++i) {
		auto old = bit_get(i);

		if(old == value) {
			kpanic();
		}

		bit_set(i, value);
		if(value) {
			m_used++;
		} else {
			m_used--;
		}
	}
}

KOptional<size_t> PhysBitmap::find_one() {
	auto base = m_base.as<uint8_t>();
	auto ptr = base;
	while(ptr - base < bitmap_size()) {
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

KOptional<size_t> PhysBitmap::find_many(size_t) {
	ASSERT_NOT_REACHED();
}
