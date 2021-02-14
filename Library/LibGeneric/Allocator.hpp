#pragma once
#include <stdint.h>
#include <stddef.h>
#include <LibGeneric/Move.hpp>

#ifdef __is_kernel_build__
#include <Kernel/Debug/kpanic.hpp>
#include <Kernel/Memory/KHeap.hpp>
#endif

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
	template<class T>
	struct Allocator {
		using pointer = T*;
		using size_type = size_t;

#ifdef __is_kernel_build__
		static pointer allocate(size_type n) {
			return reinterpret_cast<pointer>(KHeap::allocate(sizeof(T)*n));
		}

		static void deallocate(pointer p, size_type n) {
			KHeap::free(p, sizeof(T) * n);
		}
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

		static pointer allocate(size_type n) {
			return Alloc::allocate(n);
		}

		static void deallocate(pointer p, size_type n) {
			Alloc::deallocate(p,n);
		}

		template<class T, class... Args>
		static void construct(Alloc&, T* p, Args&&... args) {
			::new(static_cast<void*>(p)) T(gen::forward<Args>(args)...);
		}

		template<class T>
		static void destroy(Alloc&, T* p) {
			p->~T();
		}
	};

}
