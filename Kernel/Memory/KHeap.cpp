#include <Core/IRQ/InterruptDisabler.hpp>
#include <Core/Log/Logger.hpp>
#include <Core/MP/MP.hpp>
#include <Memory/Allocators/SlabAllocator.hpp>
#include <Memory/KHeap.hpp>
#include <Memory/Units.hpp>
#include <Memory/VMM.hpp>

KHeap KHeap::s_instance {};
CREATE_LOGGER("kheap", core::log::LogLevel::Debug);

void KHeap::init() {
	log.info("Initializing kernel allocators..");

	const auto size = 1 * Units::MiB;
	auto chunk_space = VMM::allocate_kernel_heap(size);
	ENSURE(chunk_space != nullptr);
	log.debug("ChunkAllocator({}), size {}", chunk_space, size);
	m_chunk_allocator = liballoc::ChunkAllocator {
		liballoc::Arena {chunk_space, size}
	};

	for(unsigned i = 0; i < 2; ++i) {
		for(unsigned object_size = 8; object_size <= 256; object_size <<= 1) {
			slab_grow(object_size);
		}
	}
}

void KHeap::dump_stats() {
	core::irq::InterruptDisabler irq_disabler {};
	m_heap_lock.lock();

	for(auto& size_slabs : m_slab_allocators) {
		for(auto& v : size_slabs) {
			log.debug("Slab({}): size={}, free={}, used={}", v.pool_base(), v.object_size(), v.objects_free(),
			          v.objects_used());
		}
	}
	m_heap_lock.unlock();
}

/*
 *  Allocates an object with the specified size using chunked allocators (similar to alloc-family
 *  functions in userland)
 */
void* KHeap::chunk_alloc(size_t size) {
	auto* thread = this_cpu()->current_thread();

	if(thread) {
		thread->preempt_disable();
	}
	m_heap_lock.lock();
	auto* ptr = m_chunk_allocator.allocate(size);
	m_heap_lock.unlock();
	if(thread) {
		thread->preempt_enable();
	}

	return ptr;
}

/*
 *  Frees an object allocated using chunk_alloc.
 */
void KHeap::chunk_free(void* ptr) {
	auto* thread = this_cpu()->current_thread();

	if(thread) {
		thread->preempt_disable();
	}
	m_heap_lock.lock();
	m_chunk_allocator.free(ptr);
	m_heap_lock.unlock();
	if(thread) {
		thread->preempt_enable();
	}
}

/*
 *  Allocates an object of the given size using slab allocators.
 *  Slabs use predetermined object sizes to speed up allocations of commonly used structs.
 */
void* KHeap::slab_alloc(size_t size) {
	auto* thread = this_cpu()->current_thread();

	if(thread) {
		thread->preempt_disable();
	}
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
	if(thread) {
		thread->preempt_enable();
	}

	return ptr;
}

/*
 *  Frees an object allocated by a slab allocator.
 */
void KHeap::slab_free(void* ptr, size_t size) {
	auto* thread = this_cpu()->current_thread();

	if(thread) {
		thread->preempt_disable();
	}
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
	if(thread) {
		thread->preempt_enable();
	}
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
	auto slab = maybe_slab.unwrap();
	log.debug("SlabAllocator({}), size {}, objects {}", slab.pool_base(), slab.object_size(), slab.objects_free());

	auto& list = m_slab_allocators[index_for_size(requested_size)];
	list.push_back(gen::move(slab));
	return &list.back();
}
