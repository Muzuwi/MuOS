#pragma once
namespace gen {
	template<class T>
	struct remove_reference {
		typedef T type;
	};

	template<class T>
	struct remove_reference<T&> {
		typedef T type;
	};

	template<class T>
	struct remove_reference<T&&> {
		typedef T type;
	};

	template<class T>
	struct remove_pointer {
		typedef T type;
	};

	template<class T>
	struct remove_pointer<T*> {
		typedef T type;
	};

	template<class T>
	struct remove_pointer<T* const> {
		typedef T type;
	};

	template<class T>
	struct remove_pointer<T* volatile> {
		typedef T type;
	};

	template<class T>
	struct remove_pointer<T* const volatile> {
		typedef T type;
	};

	template<class T>
	constexpr typename gen::remove_reference<T>::type&& move(T&& a) {
		return static_cast<typename gen::remove_reference<T>::type&&>(a);
	}

	template<class T>
	T&& forward(typename gen::remove_reference<T>::type& t) noexcept {
		return static_cast<T&&>(t);
	}

	template<class T>
	T&& forward(typename gen::remove_reference<T>::type&& t) noexcept {
		return static_cast<T&&>(t);
	}
}