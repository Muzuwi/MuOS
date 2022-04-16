#pragma once
#include <LibGeneric/Allocator.hpp>
#include <LibGeneric/Move.hpp>

namespace gen {

	template<class T, class... Args>
	constexpr T* construct_at(T* ptr, Args&&... args) {
		return ::new(const_cast<void*>(static_cast<const volatile void*>(ptr))) T(gen::forward<Args>(args)...);
	}

	template<class T>
	constexpr void destroy_at(T* ptr) {
		if(!ptr) {
			return;
		}

		ptr->~T();
	}
}
