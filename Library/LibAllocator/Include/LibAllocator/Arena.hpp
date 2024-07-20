#pragma once
#include <stddef.h>
#include <stdint.h>

namespace liballoc {
	struct Arena {
		constexpr Arena(void* start, size_t len)
		    : base(start)
		    , length(len) {}

		[[nodiscard]] void* end() const { return reinterpret_cast<char*>(base) + length; }

		void* const base;
		const size_t length;
	};
}