#pragma once
#include <Core/Assert/Assert.hpp>
#include <LibGeneric/Spinlock.hpp>
#include <LibGeneric/StaticVector.hpp>
#include <Memory/Allocators/ChunkAllocator.hpp>
#include <Memory/Allocators/SlabAllocator.hpp>
#include "LibGeneric/Memory.hpp"

class KHeap {
	template<class T>
	struct ChunkAllocWrapper {
		using pointer = T*;
		using size_type = size_t;
		using const_pointer = const T*;

		static pointer allocate(size_type n) {
			auto* ptr = KHeap::instance().chunk_alloc(sizeof(T) * n);
			return reinterpret_cast<pointer>(ptr);
		}

		static void deallocate(pointer p, size_type) { KHeap::instance().chunk_free(p); }

		template<class Type>
		struct rebind {
			using other = ChunkAllocWrapper<Type>;
		};
	};

	static KHeap s_instance;

	KHeap() noexcept = default;

	static constexpr size_t index_for_size(size_t n) {
		if(n <= 8) {
			return 0;
		} else if(n <= 16) {
			return 1;
		} else if(n <= 32) {
			return 2;
		} else if(n <= 64) {
			return 3;
		} else if(n <= 128) {
			return 4;
		} else if(n <= 256) {
			return 5;
		} else if(n <= 512) {
			return 6;
		}
		ENSURE_NOT_REACHED();
	}

	gen::Spinlock m_heap_lock {};
	ChunkAllocator m_chunk_allocator {};
	gen::StaticVector<SlabAllocator, 256> m_slab_allocators[7] {};

	SlabAllocator* slab_grow(size_t requested_size);
public:
	static KHeap& instance() { return s_instance; }

	void init();

	void* slab_alloc(size_t size);

	void slab_free(void* ptr, size_t size);

	void* chunk_alloc(size_t size);

	void chunk_free(void* ptr);

	void dump_stats();

	static inline void* allocate(size_t size) { return instance().chunk_alloc(size); }

	static inline void free(void* ptr, size_t = 0) { return instance().chunk_free(ptr); }

	template<class T, class... Args>
	static inline T* make(Args&&... args) {
		auto* storage = allocate(sizeof(T));
		if(!storage) {
			return nullptr;
		}
		auto* obj = reinterpret_cast<T*>(storage);
		return gen::construct_at(obj, gen::forward<Args>(args)...);
	}
};
