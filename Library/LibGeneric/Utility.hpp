#pragma once

namespace gen {
	template<typename Target, typename Source>
	constexpr auto bitcast(Source const& v) -> Target {
		static_assert(sizeof(Target) == sizeof(Source), "Type arguments for bitcast must be the same size.");
		auto target = Target();
		__builtin_memcpy(&target, &v, sizeof(Target));
		return target;
	}
}