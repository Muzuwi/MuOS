#pragma once

namespace gen {
	template<class T>
	struct less {
		constexpr bool operator()(T const& lhs, T const& rhs) { return lhs < rhs; }
	};

	template<class T>
	struct greater {
		constexpr bool operator()(T const& lhs, T const& rhs) { return lhs > rhs; }
	};

	template<class T>
	struct less_eq {
		constexpr bool operator()(T const& lhs, T const& rhs) { return lhs <= rhs; }
	};

	template<class T>
	struct greater_eq {
		constexpr bool operator()(T const& lhs, T const& rhs) { return lhs >= rhs; }
	};
}