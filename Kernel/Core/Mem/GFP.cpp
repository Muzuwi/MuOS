#include <Arch/VM.hpp>
#include <Core/Error/Error.hpp>
#include <Core/Log/Logger.hpp>
#include <Core/Mem/GFP.hpp>
#include <Core/Mem/Layout.hpp>
#include <LibAllocator/Arena.hpp>
#include <LibAllocator/ChunkAllocator.hpp>
#include <LibAllocator/SlabAllocator.hpp>
#include <LibFormat/Formatters/Pointer.hpp>
#include <LibGeneric/LockGuard.hpp>
#include <LibGeneric/Memory.hpp>
#include <LibGeneric/Optional.hpp>
#include <LibGeneric/Spinlock.hpp>

CREATE_LOGGER("core::mem::gfp", core::log::LogLevel::Debug);

struct AllocatorBase {
	//  Pointer to the next allocator
	AllocatorBase* next;
	//  Pointer to the start of the allocator region
	void* start;
	//  Size of the allocator region
	//  This is not an indicator of how much memory is availble, but is
	//  used to determine the allocator that a given block is allocated in.
	size_t size;

	//  Handle a given allocation request
	virtual void* allocate(size_t order, core::mem::PageAllocFlags flags) = 0;
	//  Free an allocation that was previously given out by this allocator
	virtual core::Error free(core::mem::PageAllocation) = 0;
};

struct SlabBasedPageAllocator : public AllocatorBase {
	SlabBasedPageAllocator(liballoc::Arena arena)
	    : slab(liballoc::Arena { idmap(arena.base), arena.length }, 0x1000) {
		AllocatorBase::start = arena.base;
		AllocatorBase::size = arena.length;
	}

	void* allocate(size_t order, core::mem::PageAllocFlags) override {
		ENSURE(order == 0);
		auto* ptr = slab.allocate();
		if(!ptr) {
			return nullptr;
		}
		return idunmap(ptr);
	}

	core::Error free(core::mem::PageAllocation alloc) override {
		ENSURE(alloc.order == 0);
		slab.free(idmap(alloc.base));
		return core::Error::Ok;
	}

	liballoc::SlabAllocator slab;
};

//  Protects all GFP data
static gen::Spinlock s_lock {};
//  Root physical memory allocator, initialized at boot time
static AllocatorBase* s_root {};

/*	Find the next allocator of a given size that can potentially satisfy a
 * 	given allocation request for `order`. This does not mean that the
 * 	allocator will actually be able to fulfill the request due to alignment
 * 	and simply not having enough memory, but allows us to quickly filter
 * 	out allocators that will never be able to give us what we want.
 */
static AllocatorBase* find_next_allocator_satisfying_order(AllocatorBase* start_at, size_t order) {
	if(!start_at) {
		return nullptr;
	}

	const auto request_size = core::mem::order_to_size(order);
	auto* current = start_at;
	while(current) {
		if(current->size >= request_size) {
			return current;
		}
		current = current->next;
	}

	return nullptr;
}

/*	Find the allocator that was used for allocating the given request.
 */
static AllocatorBase* find_allocator_for_allocation(core::mem::PageAllocation allocation) {
	auto* current = s_root;
	while(current) {
		if(allocation.base >= current->start && allocation.size() <= current->size) {
			return current;
		}
		current = current->next;
	}
	return nullptr;
}

/*	Allocate an object of a given type `T` using the allocalloc heap.
 * 	The object is constructed in place using the given constructor
 * 	arguments and a pointer to it is returned.
 */
template<typename T, typename... Args>
static T* allocalloc(Args... args) {
	//  Allocator for allocating the allocators (yo dawg)
	static gen::Optional<liballoc::ChunkAllocator> s_allocalloc {};

	//  The allocator isn't initialized yet.
	//  This will happen exclusively at boot time, at which
	//  point the initial allocalloc region is requested.
	//	Avoiding dependency on the main kernel heap makes GFP semi-freestanding,
	//	allowing GFP usage almost immediately after the kernel is reached.
	if(!s_allocalloc.has_value()) {
		void* pstart;
		const auto maybe_handle = core::mem::request(CONFIG_CORE_MEM_GFP_DEFAULT_ALLOCALLOC_SIZE, 0x1000,
		                                             core::mem::RegionType::Allocator, pstart);
		if(maybe_handle.has_error()) {
			return nullptr;
		}
		//  LEAK: Leaking the region handle, as we probably won't ever need to free this
		void* vstart = idmap(pstart);
		s_allocalloc = liballoc::ChunkAllocator {
			liballoc::Arena { vstart, CONFIG_CORE_MEM_GFP_DEFAULT_ALLOCALLOC_SIZE },
		};
	}
	return gen::construct_at<T>((T*)s_allocalloc.unwrap().allocate(sizeof(T)), gen::forward<Args>(args)...);
}

/*	Create a new allocator, using an incoming allocation request as a hint,
 * 	and store it in the allocator list.
 */
static AllocatorBase* create_allocator_for_request(size_t order, core::mem::PageAllocFlags flags) {
	constexpr const size_t default_request_size = 32 * 1024 * 1024;

	//	FIXME: Currenly, ignore the request entirely and just create
	//  a new allocator of constant size.
	(void)order;
	(void)flags;

	void* pstart;
	const auto maybe_handle =
	        core::mem::request(default_request_size, 0x1000, core::mem::RegionType::Allocator, pstart);
	if(maybe_handle.has_error()) {
		return nullptr;
	}

	auto* allocator = allocalloc<SlabBasedPageAllocator>(liballoc::Arena { pstart, default_request_size });
	if(!allocator) {
		(void)core::mem::free(maybe_handle.data());
		return nullptr;
	}
	//  Link the allocator
	//  For simplicity, put the new allocator as root.
	allocator->next = s_root;
	s_root = allocator;

	return allocator;
}

[[nodiscard]] core::Result<core::mem::PageAllocation> core::mem::allocate_pages(size_t order,
                                                                                core::mem::PageAllocFlags flags) {
	gen::LockGuard lg { s_lock };

	auto* alloc = s_root;
	while(alloc) {
		alloc = find_next_allocator_satisfying_order(alloc, order);
		if(!alloc) {
			break;
		}

		auto* ptr = alloc->allocate(order, flags);
		if(!ptr) {
			//  This allocator couldn't allocate our request, try a different one
			alloc = alloc->next;
			continue;
		}
		return core::Result<core::mem::PageAllocation> {
			core::mem::PageAllocation {
			                           .base = ptr,
			                           .order = order,
			                           .flags = flags,
			                           }
		};
	}

	//  None of the allocators managed to fulfill our request, either
	//  because:
	//    1) they're all full,
	//    2) alignment requirements and fragmentation prevent allocation
	//    3) we're trying to allocate a block of size that is greater
	//		 than all the existing allocators can handle
	//  We can try and create a new allocator, while using the allocation order
	//  as a hint for how much the allocator should be able to handle.
	auto* allocator = create_allocator_for_request(order, flags);
	if(!allocator) {
		return core::Result<core::mem::PageAllocation> { core::Error::NoMem };
	}
	auto* ptr = allocator->allocate(order, flags);
	if(!ptr) {
		return core::Result<core::mem::PageAllocation> { core::Error::NoMem };
	}
	return core::Result<core::mem::PageAllocation> {
		core::mem::PageAllocation {
		                           .base = ptr,
		                           .order = order,
		                           .flags = flags,
		                           }
	};
}

void core::mem::free_pages(core::mem::PageAllocation alloc) {
	gen::LockGuard lg { s_lock };

	auto* allocator = find_allocator_for_allocation(alloc);
	if(!allocator) {
		return;
	}

	const auto err = allocator->free(alloc);
	if(err != core::Error::Ok) {
		::log.warning("BUG: Freeing allocation failed ({}), base={x} order={}", err, Format::ptr(alloc.base),
		              alloc.order);
	}
}
