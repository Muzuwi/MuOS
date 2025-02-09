#include <Core/Error/Error.hpp>
#include <Core/Mem/Layout.hpp>
#include <LibGeneric/LockGuard.hpp>
#include <LibGeneric/Optional.hpp>
#include <LibGeneric/Spinlock.hpp>
#include <string.h>
#include <SystemTypes.hpp>

struct MmRegionData {
	core::mem::Region region;
	core::mem::RegionType old_type;
};

//  Protects all below data
static constinit gen::Spinlock s_lock {};
//  Underlying storage for all regions
//  For simplicity, use a statically allocated array for now.
static constinit MmRegionData s_regions[CONFIG_CORE_MEM_LAYOUT_MAX_REGIONS] = {};
//  How many regions were created so far?
static constinit size_t s_region_count = {};

static MmRegionData* find_region_by_handle(core::mem::RegionHandle handle) {
	for(size_t i = 0; i < s_region_count && i < CONFIG_CORE_MEM_LAYOUT_MAX_REGIONS; ++i) {
		if(s_regions[i].region.start == handle) {
			return &s_regions[i];
		}
	}
	return nullptr;
}

static inline bool segments_intersect_end_exclusive(void* x1, void* x2, void* y1, void* y2) {
	return x2 > y1 && y2 > x1;
}

static bool create_new_region(void* start, size_t len, core::mem::RegionType type) {
	if(s_region_count >= sizeof(s_regions)) {
		return false;
	}

	auto data = MmRegionData {};
	data.region.start = start;
	data.region.len = len;
	data.region.type = type;
	data.old_type = type;
	s_regions[s_region_count++] = data;

	return true;
}

static void sort_regions() {
	auto* ptr = s_regions;
	//  Not optimal at all, but it'll do for now
	for(int64 i = 1; i < static_cast<int64>(s_region_count); ++i) {
		for(int64 j = i; j > 0 && ptr[j - 1].region.start > ptr[j].region.start; j--) {
			auto temp = ptr[j];
			ptr[j] = ptr[j - 1];
			ptr[j - 1] = temp;
		}
	}
}

static void combine_adjacent_equivalent_regions() {
	if(s_region_count <= 1) {
		return;
	}

	for(size_t current = 0; current < s_region_count - 1; ++current) {
		auto& curr = s_regions[current];
		auto& next = s_regions[current + 1];
		//  If it's allocated, we can't do anything
		if(curr.region.type != core::mem::RegionType::Usable) {
			continue;
		}
		//  End of the current one must be exactly the start of the next region
		if((uint8*)curr.region.start + curr.region.len != next.region.start) {
			continue;
		}
		//  Region types must match
		if(curr.region.type != next.region.type) {
			continue;
		}

		//  The regions are equivalent - we can merge them together
		curr.region.len += next.region.len;
		//  Move everything downwards to cover the hole left by the other region
		for(size_t tmp = current + 1; tmp < s_region_count - 1; ++tmp) {
			s_regions[tmp] = s_regions[tmp + 1];
		}
		//  Evil loop var modification
		--s_region_count;
	}
}

/*	Find a region index for a given range request.
 *
 * 	A region fulfills the given request if the entire request is a
 * 	subsegment of the region segment.
 */
static gen::Optional<size_t> find_region_for_request(void* request_start, size_t request_len) {
	for(size_t idx = 0; idx < s_region_count; ++idx) {
		auto& region = s_regions[idx];
		void* request_end = (uint8*)request_start + request_len;

		if(request_start >= region.region.start && request_end <= region.region.end()) {
			return { idx };
		}
	}

	return gen::nullopt;
}

/*	Ensure the region list's preconditions are satisfied.
 *
 * 	After modifying the region list, you MUST call this function.
 * 	The preconditions are:
 * 		1) All regions are sorted in address ascending order
 * 		2) Consecutive regions are either of different type, or they
 * 		   have a hole between them
 * 		3) As a consequence of 2), contiguous memory regions are combined
 */
static void on_region_change() {
	//  Ensure the regions are sorted and combine adjacent ones
	sort_regions();
	combine_adjacent_equivalent_regions();
}

/*	Split an existing region and create a new one based on
 * 	the given reservation request.
 */
static bool split_and_create_region(void* start, size_t len, core::mem::RegionType new_type, MmRegionData& existing) {
	//  Determine the intersection type
	//  As the requested range is a subset of the region range, we have to handle
	//  these cases;
	//  1) Hole to the left of the request
	//  2) Hole to the right of the request
	//  3) Hole to the left and right of the request
	//  4) Request covers the entire range

	void* end = reinterpret_cast<char*>(start) + len;
	if(start == existing.region.start && end == existing.region.end()) {
		//  4), simply update the type
		existing.old_type = existing.region.type;
		existing.region.type = new_type;
	} else {
		const auto left_len = static_cast<char*>(start) - static_cast<char*>(existing.region.start);
		const auto right_len = static_cast<char*>(existing.region.end()) - static_cast<char*>(end);

		//  Has left hole
		if(left_len > 0) {
			//  Create a region for <existing.start,new.start) of existing.type
			if(!create_new_region(existing.region.start, left_len, existing.region.type)) {
				return false;
			}
		}

		//  Has right hole
		if(right_len > 0) {
			//  Create a region for <new.end,existing.end) of existing.type
			if(!create_new_region(end, right_len, existing.region.type)) {
				//  Clean up after left_len creation if we fail here to provide "atomicity"
				//  We haven't sorted the regions yet, so we simply need to decrement the count
				//  to get rid of the one we've just created.
				if(left_len > 0) {
					s_region_count--;
				}
				return false;
			}
		}

		//  Shrink, move and change the type of the existing region
		existing.region.start = start;
		existing.region.len = existing.region.len - left_len - right_len;
		existing.old_type = existing.region.type;
		existing.region.type = new_type;
	}

	//  Ensure the structure's preconditions are kept
	//  WARNING: This may invalidate the `existing` reference, `handle` is
	//  saved before doing this to prevent issues (as the handle is just a
	//  unique identifier which never changes).
	on_region_change();

	return true;
}

core::Result<core::mem::RegionHandle> core::mem::request_range(void* start, size_t len, RegionType new_type) {
	//  You cannot request a region to be usable - that's called freeing
	if(new_type == RegionType::Usable) {
		return core::Result<RegionHandle> { Error::InvalidArgument };
	}

	gen::LockGuard lg { s_lock };

	//  Find a region for the given request.
	//  As subsequent regions of Usable type are merged, if the request covers
	//  two or more regions, then we can't do anything - at least part of the requested
	//  physical address space was already allocated/reserved before.
	auto maybe_region = find_region_for_request(start, len);
	if(!maybe_region) {
		return core::Result<RegionHandle> { Error::InvalidArgument };
	}
	auto& existing = s_regions[maybe_region.unwrap()];

	//  If the region is not already marked as Usable, we can't allocate it
	if(existing.region.type != RegionType::Usable) {
		return core::Result<RegionHandle> { Error::InvalidArgument };
	}

	if(!split_and_create_region(start, len, new_type, existing)) {
		return core::Result<core::mem::RegionHandle> { core::Error::NoMem };
	}
	return core::Result<core::mem::RegionHandle> { start };
}

core::Result<core::mem::RegionHandle> core::mem::request(size_t length, size_t alignment, RegionType new_type,
                                                         void*& start) {
	//  You cannot request a region to be usable - that's called freeing
	if(new_type == RegionType::Usable) {
		return core::Result<RegionHandle> { Error::InvalidArgument };
	}

	gen::LockGuard lg { s_lock };

	void* new_start = nullptr;
	MmRegionData* region_to_update = nullptr;
	for(size_t current = 0; current < s_region_count; ++current) {
		auto& region = s_regions[current].region;

		//  Only usable regions
		if(region.type != core::mem::RegionType::Usable) {
			continue;
		}
		//  Check if the region is big enough
		if(region.len < length) {
			continue;
		}

		//  Check alignment
		//	Even if the start of the region is not directly aligned, we can
		//	still allocate only part of the region, so check if aligning the
		//  start upwards gives us a range that is still within bounds.
		if((uintptr_t)region.start & (uintptr_t)(alignment - 1)) {
			auto* aligned_start = region.start;
			const auto padding = gen::align(alignment, aligned_start);
			if(padding + length > region.len) {
				continue;
			}
			new_start = aligned_start;
		} else {
			new_start = region.start;
		}
		region_to_update = &s_regions[current];
		break;
	}

	if(!region_to_update) {
		//  If we didn't find anything, bail out - there is no free memory left
		return core::Result<core::mem::RegionHandle> { core::Error::NoMem };
	}

	if(!split_and_create_region(new_start, length, RegionType::Allocator, *region_to_update)) {
		return core::Result<core::mem::RegionHandle> { core::Error::NoMem };
	}
	start = new_start;
	return core::Result<core::mem::RegionHandle> { start };
}

core::Error core::mem::free(RegionHandle handle) {
	gen::LockGuard lg { s_lock };

	auto* data = find_region_by_handle(handle);
	if(!data) {
		return Error::InvalidArgument;
	}

	data->region.type = RegionType::Usable;
	on_region_change();

	return Error::Ok;
}

void core::mem::for_each_region(ForEachRegionCb cb) {
	gen::LockGuard lg { s_lock };
	for(size_t i = 0; i < s_region_count; ++i) {
		cb(s_regions[i].region);
	}
}

core::Error core::mem::create(void* start, size_t len, RegionType type) {
	gen::LockGuard lg { s_lock };

	for(size_t current = 0; current < s_region_count; ++current) {
		void* end = static_cast<char*>(start) + len;
		auto& other = s_regions[current];

		//  Validate that the newly created region does not overlap with any other ones
		if(!segments_intersect_end_exclusive(start, end, other.region.start, other.region.end())) {
			continue;
		}

		//  At least one region overlaps the region we're trying to create, bail out
		return Error::InvalidArgument;
	}

	if(!create_new_region(start, len, type)) {
		return Error::NoMem;
	}
	on_region_change();

	return Error::Ok;
}
