#pragma once
#include <Core/Error/Error.hpp>
#include <SystemTypes.hpp>

/* Size of the allocalloc region used for storing allocator metadata */
#define CONFIG_CORE_MEM_GFP_DEFAULT_ALLOCALLOC_SIZE (32768)

namespace core::mem {
	/* Convert a GFP order to a size in bytes */
	constexpr size_t order_to_size(size_t order) {
		return (1ul << order) * 0x1000ul;
	}
	/* Convert a size in bytes to the nearest viable GFP order */
	constexpr size_t size_to_order_nearest(size_t size) {
		size_t p = 1ul;
		while(order_to_size(p) < size)
			++p;
		return p;
	}

	enum PageAllocFlags {
	};

	struct PageAllocation {
		void* base;
		size_t order;
		PageAllocFlags flags;

		[[nodiscard]] constexpr void* end() const { return static_cast<uint8*>(base) + size(); }

		[[nodiscard]] constexpr void* last() const { return static_cast<uint8*>(end()) - 1; }

		[[nodiscard]] constexpr size_t size() const { return order_to_size(order); }
	};

	[[nodiscard]] core::Result<PageAllocation> allocate_pages(size_t order, PageAllocFlags);
	void free_pages(PageAllocation);

}