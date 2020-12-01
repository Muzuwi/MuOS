#pragma once
namespace gen {
	template<class T>
	struct remove_reference     { typedef T type; };

	template<class T>
	struct remove_reference<T&> { typedef T type; };

	template<class T>
	struct remove_reference<T&&>{ typedef T type; };

	template<class T>
	constexpr typename gen::remove_reference<T>::type&& move(T&& a) {
		return static_cast<typename gen::remove_reference<T>::type&&>(a);
	}
}