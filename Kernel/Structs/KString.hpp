#pragma once
#include <LibFormat/Format.hpp>
#include <LibGeneric/String.hpp>
#include <SystemTypes.hpp>

namespace str {
	/** Append the hex representation of a given byte to
	 *  a string.
	 */
	inline void append_hex(gen::String& str, uint8 byte) {
		char buf[8];
		Format::format("{x}", buf, sizeof(buf), byte);
		str += buf;
	}
}
