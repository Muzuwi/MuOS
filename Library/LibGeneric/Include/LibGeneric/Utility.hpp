#pragma once

namespace gen {
	template<typename Target, typename Source>
	constexpr auto bitcast(Source const& v) -> Target {
		static_assert(sizeof(Target) == sizeof(Source), "Type arguments for bitcast must be the same size.");
		return __builtin_bit_cast(Target, v);
	}
}