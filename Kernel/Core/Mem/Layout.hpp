#pragma once
#include <Core/Error/Error.hpp>
#include <Structs/KFunction.hpp>
#include <SystemTypes.hpp>

//  How many unique physical memory regions can be handled,
//  regardless of if they're used or not.
#define CONFIG_CORE_MEM_LAYOUT_MAX_REGIONS (32)

/*  core::mem physical memory layout management.
 *
 *  This subsystem manages allocation of regions in the physical address space.
 */
namespace core::mem {
	enum class RegionType {
		/* non-reserved memory, usable for any purpose by the kernel */
		Usable = 0,
		/* hardware reserved memory, specified for example in the DTS */
		HardwareReservation = 1,
		/* used by the kernel for page allocators */
		Allocator = 2,
		/* the platform reports that this range is defectve */
		Defective = 3,
	};

	inline char const* type_to_str(RegionType type) {
		switch(type) {
			case RegionType::Usable: return "Usable";
			case RegionType::HardwareReservation: return "HardwareReservation";
			case RegionType::Allocator: return "Allocator";
			default: return "INVALID";
		}
	}

	struct Region {
		void* start;
		size_t len;
		RegionType type;

		[[nodiscard]] constexpr void* end() const { return static_cast<char*>(start) + len; }
	};

	using RegionHandle = void*;
	using ForEachRegionCb = KFunction<void(Region)>;

	/*	Request a region of physical memory with the given size and alignment.
	 *
	 * 	This finds and reserves a range of physical memory that satisfies the
	 *	given size and alignment constraints. `alignment` must be a power-of-two,
	 * 	otherwise the behavior is undefined. `new_type` specifies the purpose of
	 * 	the reservation - this doesn't affect the allocation process, but is only
	 * 	used for kernel bookkeeping.
	 *
	 *	On success, `start` is modified to contain the starting *physical* address of
	 *  the newly allocated region. The starting address of the region will have the
	 * 	alignment as specified by `alignment`, and will span `length` bytes.
	 * 	Additionally, a handle is returned that can be used to free the range once
	 * 	it is no longer required.
	 *
	 * 	NOTE: This should not be used for generic allocations but for allocating ranges
	 * 	that are given out by individual allocators themselves. While you could theoretically
	 * 	use this to allocate a single page at a time, the layout subsystem is simplified
	 *	and you will eventually hit the internal limit imposed by CONFIG_CORE_MEM_LAYOUT_MAX_REGIONS,
	 * 	thereby preventing ANY physical memory allocations entirely. The layout subsystem
	 * 	is NOT designed for single-page allocations.
	 */
	core::Result<RegionHandle> request(size_t length, size_t alignment, RegionType new_type, void*& pstart);

	/*  Request allocation of the specified physical memory region.
	 *
	 *  "Allocation" here does not mean allocation of pages, but rather allocating
	 *  the region of physical memory itself for the purposes of a different kernel subsys.
	 *  `start` and `len` specify the region that is being requested. `type` describes the
	 *  purpose of the allocation - see the enum definition for explanations. Only Usable
	 * 	regions (or subregions thereof) can be requested.
	 */
	core::Result<RegionHandle> request_range(void* start, size_t len, RegionType new_type);

	/*  Free a physical memory region that was previously allocated using core::mem::request().
	 * 	After freeing, the region is returned to Usable state and can be allocated once again.
	 */
	core::Error free(RegionHandle);

	/*  Iterate over every region of the physical memory layout.
	 *
	 *  Allows introspection into the regions that were created so far.
	 *  WARNING: For the duration of the iteration, the address space layout
	 *  lock is held - avoid long blocking operations from the callback!
	 */
	void for_each_region(ForEachRegionCb);

	/*  Create a brand new region.
	 *
	 *  This is used by low-level boot procedures, you should never have a reason to use this!
	 *  Creation is only possible for a completely non-overlapping region (i.e, for the purposes
	 *  of creating the initial memory layout during boot). If the new region overlaps at all with
	 *  existing regions, the process will fail.
	 */
	core::Error create(void* start, size_t len, RegionType);
}