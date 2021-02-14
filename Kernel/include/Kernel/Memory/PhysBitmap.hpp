#pragma once
#include <string.h>
#include <Kernel/SystemTypes.hpp>
#include <Kernel/Memory/Ptr.hpp>
#include <Kernel/KOptional.hpp>

class PhysBitmap {
	PhysAddr m_base;
	size_t m_entries;
	size_t m_used;

	static inline int64_t divround(const int64_t n, const int64_t d) {
		return ((n < 0) ^ (d < 0)) ? ((n - d/2)/d) : ((n + d/2)/d);
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

	//  Quick skip through full bytes
	KOptional<size_t> find_one() {
		auto base = m_base.as<uint8_t>();
		auto ptr = base;
		while (ptr - base < bitmap_size()) {
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

	KOptional<size_t> find_many(size_t count) {
		ASSERT_NOT_REACHED();

		return KOptional<size_t>{};
	}

	void mark_bits(size_t idx, size_t count, bool value) {
		for(size_t i = idx; i < idx + count; ++i) {
			auto old = bit_get(i);

			if(old == value) {
				kerrorf("PageBitmapAllocator: Corrupted state or double free for idx=%i\n", i);
				continue;
			}

			bit_set(i, value);
			if(value)
				m_used++;
			else
				m_used--;
		}
	}

	KOptional<size_t> _allocate(size_t count) {
		auto ret = (count == 1) ? find_one()
								: find_many(count);
		if(!ret.has_value())
			return KOptional<size_t>{};

		mark_bits(ret.unwrap(), count, true);
		return ret;
	}

	void _free(size_t idx, size_t count) {
		//  Mark bits taken by the allocation as unused
		mark_bits(idx, count, false);
	}
public:
	PhysBitmap() noexcept
	: m_base(nullptr), m_entries(0), m_used(0) {}

	PhysBitmap(PhysAddr physical, size_t entries) noexcept
	: m_base(physical), m_entries(entries), m_used(0) {
		memset(physical.get_mapped(), 0, divround(m_entries, 8));
	}

	size_t used() const {
		return m_used;
	}

	size_t entries() const {
		return m_entries;
	}

	size_t bitmap_size() const {
		return divround(m_entries, 8);
	}

	KOptional<size_t> allocate(size_t count = 1) {
		return _allocate(count);
	}

	void free(size_t idx, size_t alloc_size) {
		_free(idx, alloc_size);
	}
};