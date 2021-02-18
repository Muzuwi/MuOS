#include <Kernel/Memory/PhysBitmap.hpp>
#include <Kernel/Memory/Allocators/SlabAllocator.hpp>
#include <Kernel/Memory/PMM.hpp>
#include <Kernel/Memory/VMM.hpp>
#include <Kernel/Memory/KHeap.hpp>
#include <Kernel/Debug/kdebugf.hpp>
#include <Kernel/Debug/kpanic.hpp>

#include <LibGeneric/List.hpp>
#include <Kernel/Memory/kmalloc.hpp>
#include <LibGeneric/Vector.hpp>

using gen::List;

static List<SlabAllocator,KMalloc::BootstrapAllocator> s_slab_allocators[7];

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

void* KHeap::allocate(size_t n) {
	auto& slabs = s_slab_allocators[index_for_size(n)];
	for(auto& allocator : slabs) {
		if(allocator.objects_free() > 0) {
			auto ptr = allocator.allocate();
			//kdebugf("[KHeap] Alloc ptr=%x%x, size=%i, overcommit=%i\n", (uintptr_t)ptr>>32u, (uintptr_t)ptr&0xffffffffu, n, allocator.object_size() - n);
			return ptr;
		}
	}

	kerrorf("[KHeap] Allocation request failed for size=%i\n", n);
	return nullptr;
}

void KHeap::free(void* p, size_t n) {
	if(!p) return;

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

	static constexpr const unsigned pool_order = 0;

	//  TODO: Heap base address randomization
	void* virtual_address = &_ukernel_heap_start;
	for(unsigned object_size = 4; object_size <= 256; object_size <<= 1) {
		SlabAllocator slab{object_size, pool_order};
		auto& slabs = s_slab_allocators[index_for_size(object_size)];
		slabs.push_back(slab);
		slabs.back().initialize(virtual_address);

		kdebugf("[KHeap] SlabAllocator(%i) at virtual %x%x, objects %i\n", object_size, (uintptr_t)virtual_address>>32u, (uintptr_t)virtual_address&0xffffffffu, slabs.back().objects_free());

		virtual_address = (void*)(((uintptr_t)virtual_address + slabs.back().virtual_size() + 0x1000) & ~(0x1000-1));
	}
}