#pragma once

namespace gen {
	template<bool B, class T = void>
	struct __enable_if {};

	template<class T>
	struct __enable_if<true, T> {
		typedef T type;
	};

	template<bool B, class T = void>
	using enable_if_t = typename gen::__enable_if<B, T>::type;

	template<typename A, typename B>
	struct __IsSameType {
		static constexpr bool value = false;
	};

	template<typename T>
	struct __IsSameType<T, T> {
		static constexpr bool value = true;
	};

	template<typename A, typename B>
	concept MustBeSameType = __IsSameType<A, B>::value;
}