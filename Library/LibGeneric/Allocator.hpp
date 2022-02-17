#pragma once

#include <LibGeneric/Move.hpp>
#include <stddef.h>
#include <stdint.h>

/*
 *  Placement new implementation
 */
[[nodiscard]] inline constexpr void* operator new(size_t, void* ptr) noexcept {
	return ptr;
}

[[nodiscard]] inline constexpr void* operator new[](size_t, void* ptr) noexcept {
	return ptr;
}

namespace gen {
#ifdef __is_kernel_build__

	void* __kernel_alloc(size_t n);

	void __kernel_free(void* p, size_t n);

#endif

	template<class T>
	struct Allocator {
		using pointer = T*;
		using size_type = size_t;

#ifdef __is_kernel_build__

		static pointer allocate(size_type n) { return reinterpret_cast<pointer>(__kernel_alloc(sizeof(T) * n)); }

		static void deallocate(pointer p, size_type n) { __kernel_free(p, sizeof(T) * n); }

#else
		static pointer allocate(size_type);
		static void deallocate(pointer, size_type);
#endif

		template<class Type>
		struct rebind {
			using other = Allocator<Type>;
		};
	};

	template<typename Alloc>
	struct AllocatorTraits {
		using allocator_type = Alloc;
		using pointer = typename Alloc::pointer;
		using size_type = typename Alloc::size_type;

		static pointer allocate(size_type n) { return Alloc::allocate(n); }

		static void deallocate(pointer p, size_type n) { Alloc::deallocate(p, n); }

		template<class T, class... Args>
		static void construct(Alloc&, T* p, Args&&... args) {
			if(p == nullptr) {
				return;
			}

			::new(static_cast<void*>(p)) T(gen::forward<Args>(args)...);
		}

		template<class T>
		static void destroy(Alloc&, T* p) {
			if(p == nullptr) {
				return;
			}

			p->~T();
		}
	};

}
