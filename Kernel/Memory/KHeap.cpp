#include <Kernel/Memory/Allocators/SlabAllocator.hpp>
#include <Kernel/Memory/PMM.hpp>
#include <Kernel/Memory/KHeap.hpp>
#include <Kernel/Memory/kmalloc.hpp>
#include <Kernel/Debug/kdebugf.hpp>
#include <Kernel/Debug/kpanic.hpp>
#include <LibGeneric/List.hpp>
#include <LibGeneric/Spinlock.hpp>
#include <LibGeneric/LockGuard.hpp>

using gen::List;

static List<SlabAllocator,KMalloc::BootstrapAllocator> s_slab_allocators[7];
static gen::Spinlock s_kheap_lock {};
static void* s_last_heap_virtual {};


static size_t index_for_size(size_t n) {
	if(n <= 4)
		return 0;
	else if(n <= 8)
		return 1;
	else if(n <= 16)
		return 2;
	else if(n <= 32)
		return 3;
	else if(n <= 64)
		return 4;
	else if(n <= 128)
		return 5;
	else if(n <= 256)
		return 6;
	//  FIXME
	kpanic();
}


/*
 *  Creates a new SlabAllocator for the given object size
 */
static SlabAllocator* grow_heap(size_t object_size) {
	static constexpr const unsigned pool_order = 0;

	SlabAllocator slab{object_size, pool_order};
	auto& slabs = s_slab_allocators[index_for_size(object_size)];
	auto it = slabs.insert(slabs.end(), slab);
	(*it).initialize(s_last_heap_virtual);

	kdebugf("[KHeap] SlabAllocator(%i) at virtual %x%x, objects %i\n", object_size, (uintptr_t)s_last_heap_virtual>>32u, (uintptr_t)s_last_heap_virtual&0xffffffffu, slabs.back().objects_free());

	s_last_heap_virtual = (void*)(((uintptr_t)s_last_heap_virtual + slabs.back().virtual_size() + 0x1000) & ~(0x1000-1));

	return &*it;
}

void* KHeap::allocate(size_t n) {
	gen::LockGuard<gen::Spinlock> lock {s_kheap_lock};

	auto& slabs = s_slab_allocators[index_for_size(n)];
	for(auto& allocator : slabs) {
		if(allocator.objects_free() > 0) {
			auto ptr = allocator.allocate();
			//kdebugf("[KHeap] Alloc ptr=%x%x, size=%i, overcommit=%i\n", (uintptr_t)ptr>>32u, (uintptr_t)ptr&0xffffffffu, n, allocator.object_size() - n);
			return ptr;
		}
	}

	//  Growth of the heap is necessary
	auto alloc = grow_heap(n);
	if(alloc) {
		auto ptr = alloc->allocate();
//		kdebugf("[KHeap] Alloc ptr=%x%x, size=%i, overcommit=%i\n", (uintptr_t)ptr>>32u, (uintptr_t)ptr&0xffffffffu, n, alloc->object_size() - n);
		return ptr;
	}

	kerrorf("[KHeap] Allocation request failed for size=%i\n", n);
	return nullptr;
}

void KHeap::free(void* p, size_t n) {
	if(!p) return;

	gen::LockGuard<gen::Spinlock> lock {s_kheap_lock};
	auto& slabs = s_slab_allocators[index_for_size(n)];
	for(auto& allocator : slabs) {
		if(allocator.contains_address(p)) {
			//kdebugf("[KHeap] Free ptr=%x%x, size=%i\n", (uintptr_t)p>>32u, (uintptr_t)p&0xffffffffu, n);
			allocator.free(p);
			return;
		}
	}

	kerrorf("[KHeap] Request to free memory [%x%x] outside active heap range\n", (uintptr_t)p>>32u, (uintptr_t)p&0xffffffffu);
}

void KHeap::init() {
	kdebugf("[KHeap] Init slabs\n");

	//  TODO: Heap base address randomization
	s_last_heap_virtual = &_ukernel_heap_start;
	for(unsigned i = 0; i < 2; ++i) {
		for(unsigned object_size = 4; object_size <= 256; object_size <<= 1) {
			grow_heap(object_size);
		}
	}
}

void KHeap::dump_stats() {
	for(auto& size_slabs : s_slab_allocators) {
		for(auto& v : size_slabs) {
			kdebugf("    size=%i, free=%i, used=%i\n", v.object_size(), v.objects_free(), v.objects_used());
		}
	}
}
