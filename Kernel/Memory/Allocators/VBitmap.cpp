#include <Core/Assert/Assert.hpp>
#include <Memory/Allocators/VBitmap.hpp>
#include <string.h>

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

		ENSURE(old != value);

		bit_set(i, value);
		if(value) {
			m_used++;
		} else {
			m_used--;
		}
	}
}

KOptional<size_t> VBitmap::find_one() {
	auto base = static_cast<uint8*>(m_base);
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
		DEBUG_ASSERT_NOT_REACHED();
	}

	return KOptional<size_t> {};
}

KOptional<size_t> VBitmap::find_many(size_t) {
	ENSURE_NOT_REACHED();
}

void VBitmap::initialize() {
	memset(m_base, 0x00, m_buffer_size);
}
