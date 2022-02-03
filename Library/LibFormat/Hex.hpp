#pragma once

#include <stdint.h>
#include <stddef.h>

namespace Format {

	constexpr char nibble_to_hex_uppercase(uint8_t value) {
		constexpr const char lookup[16] = {
				'0', '1', '2', '3',
				'4', '5', '6', '7',
				'8', '9', 'A', 'B',
				'C', 'D', 'E', 'F'
		};
		value &= 0x0F;
		return lookup[value];
	}

	constexpr char nibble_to_hex_lowercase(uint8_t value) {
		constexpr const char lookup[16] = {
				'0', '1', '2', '3',
				'4', '5', '6', '7',
				'8', '9', 'a', 'b',
				'c', 'd', 'e', 'f'
		};
		value &= 0x0F;
		return lookup[value];
	}
}