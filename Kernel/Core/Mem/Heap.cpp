#include <Core/IRQ/InterruptDisabler.hpp>
#include <Core/Mem/Heap.hpp>
#include <Core/Mem/VM.hpp>
#include <LibAllocator/Arena.hpp>
#include <LibAllocator/ChunkAllocator.hpp>
#include <LibGeneric/LockGuard.hpp>
#include <LibGeneric/Spinlock.hpp>
#include <Structs/KOptional.hpp>

//  Was the heap vmalloc region allocated and initialized already?
static bool s_allocator_initialized { false };
//  The allocator itself, currently the heap uses only a single one
//  but ideally this should keep track of multiple heap regions to
//  be able to use more memory than HEAP_DEFAULT_SIZE.
static liballoc::ChunkAllocator s_allocator {};
//  Protects all data above
static gen::Spinlock s_lock {};

//  Ensure the heap allocator region is allocated, and the allocator
//  was initialized with it. Returns a pointer to the heap allocator,
//  or nullptr on failure.
liballoc::ChunkAllocator* ensure_allocator() {
	if(s_allocator_initialized) {
		return &s_allocator;
	}

	auto mem = core::mem::vmalloc(core::mem::HEAP_DEFAULT_SIZE);
	if(!mem) {
		return nullptr;
	}

	new(&s_allocator) liballoc::ChunkAllocator {
		liballoc::Arena { mem, core::mem::HEAP_DEFAULT_SIZE }
	};
	s_allocator_initialized = true;
	return &s_allocator;
}

void* core::mem::hmalloc(size_t n) {
	core::irq::InterruptDisabler id {};
	gen::LockGuard lg { s_lock };
	auto* alloc = ensure_allocator();
	if(!alloc) [[unlikely]] {
		return nullptr;
	}
	return alloc->allocate(n);
}

void core::mem::hfree(void* ptr) {
	core::irq::InterruptDisabler id {};
	gen::LockGuard lg { s_lock };
	auto* alloc = ensure_allocator();
	if(!alloc) [[unlikely]] {
		return;
	}
	alloc->free(ptr);
}
