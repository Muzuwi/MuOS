#include <Debug/kpanic.hpp>
#include <Memory/Allocators/VBitmap.hpp>
#include <string.h>

VBitmap::VBitmap()
    : m_base(nullptr)
    , m_buffer_size(0)
    , m_entries(0)
    , m_used(0) {}

VBitmap::VBitmap(void* base, size_t entries)
    : m_base(base)
    , m_buffer_size(divround(entries, 8))
    , m_entries(entries)
    , m_used(0) {
	memset(base, 0, m_buffer_size);
}

KOptional<size_t> VBitmap::allocate_impl(size_t count) {
	auto ret = (count == 1) ? find_one() : find_many(count);
	if(!ret.has_value()) {
		return KOptional<size_t> {};
	}

	mark_bits(ret.unwrap(), count, true);
	return ret;
}

void VBitmap::free_impl(size_t idx, size_t count) {
	//  Mark bits taken by the allocation as unused
	mark_bits(idx, count, false);
}

void VBitmap::mark_bits(size_t idx, size_t count, bool value) {
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

KOptional<size_t> VBitmap::find_one() {
	auto base = reinterpret_cast<uint8*>(m_base);
	auto ptr = base;
	while((size_t)(ptr - base) < buffer_size()) {
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
			return KOptional<size_t> { static_cast<size_t>(idx) };
		}
		ASSERT_NOT_REACHED();
	}

	return KOptional<size_t> {};
}

KOptional<size_t> VBitmap::find_many(size_t) {
	ASSERT_NOT_REACHED();
}
