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
}