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

	/*	Align a given pointer `ptr` at the boundary specified by `alignment`.
	 * 	The pointer is modified to be aligned, and the number of padding bytes
	 * 	that were added is returned.
	 * 	If `alignment` is not a power-of-two, the behavior is undefined.
	 */
	inline size_t align(size_t alignment, void*& ptr) {
		const auto intptr = reinterpret_cast<uintptr_t>(ptr);
		const auto aligned = (intptr - 1u + alignment) & -alignment;
		ptr = reinterpret_cast<void*>(aligned);
		return aligned - intptr;
	}
}
