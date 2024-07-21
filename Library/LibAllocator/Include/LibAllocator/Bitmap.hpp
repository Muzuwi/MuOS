#pragma once
#include <stddef.h>
#include <stdint.h>

namespace liballoc {
	/*  Given a bitmap at `base` with length `len`, finds the
	 *  index of the first available bit. Bits are counted
	 *  MSB-first, zero-indexed.
	 */
	static inline bool bitmap_find_one(void* base, size_t len, size_t& idx) {
		auto* ptr = reinterpret_cast<uint8_t*>(base);
		auto* end = ptr + len;
		while(ptr < end && *ptr == 0xFF) {
			++ptr;
		}
		if(ptr == end) {
			return false;
		}

		size_t n = (ptr - reinterpret_cast<uint8_t*>(base)) * 8;
		if(*ptr != 0) {
			for(size_t i = 0; i < 8; ++i) {
				if(*ptr & (1 << (7 - i))) {
					++n;
				} else {
					break;
				}
			}
		}
		idx = n;
		return true;
	}

	/*  Given a bitmap at `base` with length `len`, set the given
	 *  bit to a specific value. Bits are counted MSB-first, zero-indexed.
	 */
	static inline void bitmap_set(void* base, size_t len, size_t idx, bool value) {
		if(!base) {
			return;
		}

		auto* ptr = reinterpret_cast<uint8_t*>(base);
		auto* end = ptr + len;

		ptr += (idx / 8);
		if(ptr >= end) {
			return;
		}
		if(value) {
			*ptr = *ptr | (0x80ul >> (idx % 8));
		} else {
			*ptr = *ptr & ~(0x80ul >> (idx % 8));
		}
	}
}