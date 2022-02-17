#pragma once
#include <Memory/Units.hpp>
#include <stdint.h>
#include <SystemTypes.hpp>

class KMalloc {
	KMalloc();
	uint64_t m_total_allocations;
	uint64_t m_total_frees;
	uint64_t m_current_allocations;

	mem_range_t m_kmalloc_mem_range;

	void mark_range(size_t, size_t, bool);
	bool is_kmalloc_memory(void*);
public:
	static KMalloc& get();

	void init();
	void* kmalloc_alloc(size_t, size_t = 1);
	void kmalloc_free(void*);

	using Chunk = uint32_t;

	/*
	 *  This must match the size of the boostrap buffer KMalloc is initialized with
	 */
	static constexpr uint64_t pool_size() { return (1 * Units::MiB); }

	static constexpr uint64_t chunk_size() { return 2; }

	static constexpr uint64_t array_count() { return ((pool_size() / chunk_size()) / (sizeof(Chunk) * 8)); }

	template<class T>
	struct BootstrapAllocator {
		using pointer = T*;
		using size_type = size_t;
		using const_pointer = const T*;

		static pointer allocate(size_type n) {
			auto* ptr = KMalloc::get().kmalloc_alloc(sizeof(T) * n);
			return reinterpret_cast<pointer>(ptr);
		}

		static void deallocate(pointer p, size_type) { KMalloc::get().kmalloc_free(p); }

		template<class Type>
		struct rebind {
			using other = BootstrapAllocator<Type>;
		};
	};
};
