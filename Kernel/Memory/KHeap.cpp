#include <Memory/Allocators/SlabAllocator.hpp>
#include <Memory/KHeap.hpp>
#include <Memory/kmalloc.hpp>
#include <Memory/VMM.hpp>
#include <Debug/klogf.hpp>
#include <SMP/SMP.hpp>

KHeap KHeap::s_instance {};

void KHeap::init() {
	klogf_static("[KHeap] Initializing kernel allocators..\n");

	const auto size = 1 * Units::MiB;
	auto chunk_space = VMM::allocate_kernel_heap(size);
	kassert(chunk_space != nullptr);
	klogf_static("[KHeap] ChunkAllocator({}), size {}\n", chunk_space, size);
	m_chunk_allocator = ChunkAllocator { chunk_space, size };

	for(unsigned i = 0; i < 2; ++i) {
		for(unsigned object_size = 8; object_size <= 256; object_size <<= 1) {
			slab_grow(object_size);
		}
	}
}

void KHeap::dump_stats() {
	m_chunk_allocator.dump_allocator();
	for(auto& size_slabs : m_slab_allocators) {
		for(auto& v : size_slabs) {
			klogf_static("Slab({}): size={}, free={}, used={}\n", v.pool_base(), v.object_size(),
			             v.objects_free(),
			             v.objects_used());
		}
	}
}

/*
 *  Allocates an object with the specified size using chunked allocators (similar to alloc-family
 *  functions in userland)
 */
void* KHeap::chunk_alloc(size_t size) {
	auto* thread = SMP::ctb().current_thread();

	if(thread) { thread->preempt_disable(); }
	m_heap_lock.lock();
	auto* ptr = m_chunk_allocator.allocate(size);
	m_heap_lock.unlock();
	if(thread) { thread->preempt_enable(); }

	return ptr;
}

/*
 *  Frees an object allocated using chunk_alloc.
 */
void KHeap::chunk_free(void* ptr) {
	auto* thread = SMP::ctb().current_thread();

	if(thread) { thread->preempt_disable(); }
	m_heap_lock.lock();
	m_chunk_allocator.free(ptr);
	m_heap_lock.unlock();
	if(thread) { thread->preempt_enable(); }
}

/*
 *  Allocates an object of the given size using slab allocators.
 *  Slabs use predetermined object sizes to speed up allocations of commonly used structs.
 */
void* KHeap::slab_alloc(size_t size) {
	auto* thread = SMP::ctb().current_thread();

	if(thread) { thread->preempt_disable(); }
	m_heap_lock.lock();
	auto* ptr = [size, this]() -> void* {
		auto& slabs = m_slab_allocators[index_for_size(size)];
		for(auto& allocator : slabs) {
			if(allocator.objects_free() > 0) {
				return allocator.allocate();
			}
		}

		//  Create a new slab when necessary
		auto alloc = slab_grow(size);
		if(alloc) {
			return alloc->allocate();
		}
		return nullptr;
	}();
	m_heap_lock.unlock();
	if(thread) { thread->preempt_enable(); }

	return ptr;
}

/*
 *  Frees an object allocated by a slab allocator.
 */
void KHeap::slab_free(void* ptr, size_t size) {
	auto* thread = SMP::ctb().current_thread();

	if(thread) { thread->preempt_disable(); }
	m_heap_lock.lock();
	[size, ptr, this]() {
		auto& slabs = m_slab_allocators[index_for_size(size)];
		for(auto& allocator : slabs) {
			if(allocator.contains_address(ptr)) {
				allocator.free(ptr);
				return;
			}
		}
	}();
	m_heap_lock.unlock();
	if(thread) { thread->preempt_enable(); }
}

/*
 *  Create a new slab for the specified allocation request
 *  This function assumes a heap lock was already taken
 */
SlabAllocator* KHeap::slab_grow(size_t requested_size) {
	const unsigned actual_object_size = 8 << index_for_size(requested_size);

	auto maybe_slab = SlabAllocator::make(32768, actual_object_size);
	if(!maybe_slab.has_value()) {
		return nullptr;
	}

	auto& list = m_slab_allocators[index_for_size(requested_size)];
	list.push_back(maybe_slab.unwrap());

	auto& slab = list.back();
	klogf_static("[KHeap] SlabAllocator({}), size {}, objects {}\n", slab.pool_base(), slab.object_size(),
	             slab.objects_free());
	return &slab;
}
