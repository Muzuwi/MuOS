#include <Arch/riscv64/Boot0/TinyMM.hpp>
#include <SystemTypes.hpp>

//  Underlying storage for tinymm
static tinymm::Region s_regions[TINYMM_MAX_REGIONS] = {};
//  How many regions were created so far?
static size_t s_region_count = {};

static inline bool segments_intersect_end_exclusive(void* x1, void* x2, void* y1, void* y2) {
	return x2 > y1 && y2 > x1;
}

static bool create_new_region(void* start, size_t len, tinymm::RegionType type) {
	if(s_region_count >= sizeof(s_regions)) {
		return false;
	}

	auto region = tinymm::Region {};
	region.start = start;
	region.len = len;
	region.type = type;
	s_regions[s_region_count++] = region;

	return true;
}

static void sort_regions(tinymm::Region* ptr, size_t n) {
	//  Preloader should never have a significant enough region count for
	//  this to matter.
	for(int64 i = 1; i < static_cast<int64>(n); ++i) {
		for(int64 j = i; j > 0 && ptr[j - 1].start > ptr[j].start; j--) {
			auto temp = ptr[j];
			ptr[j] = ptr[j - 1];
			ptr[j - 1] = temp;
		}
	}
}

void tinymm::update_region(void* start, size_t len, tinymm::RegionType type) {
	for(size_t current = 0; current < s_region_count; ++current) {
	evilgotogoesbrr:
		void* end = static_cast<char*>(start) + len;
		auto& other = s_regions[current];

		//  If regions do not intersect, we don't need to do anything
		if(!segments_intersect_end_exclusive(start, end, other.start, other.end())) {
			continue;
		}

		//  Determine the intersection, in general we have to handle the cases:
		//  1. tinymm::Region is split into two on the left, i.e:
		//     ...------|                    <- new
		//        |-----------------|        <- existing
		//  2. tinymm::Region is split into two on the right, i.e:
		//                    |------...     <- new
		//        |-----------------|        <- existing
		//  3. tinymm::Region is split into three, as it is contained entirely within the
		//  existing one.
		//            |-------|              <- new
		//        |-----------------|        <- existing
		//  4. The regions are equal
		//        |-----------------|        <- new
		//        |-----------------|        <- existing
		//  5. The region is completely swallowed by another
		//     ...-------------------...	 <- new
		//        |-----------------|        <- existing
		//	   This one requires careful handling.

		//  4) must be handled first
		if(start == other.start && end == other.end()) {
			//  Only need to update the type
			other.type = type;
		}
		//  1)
		else if(end > other.start && start <= other.start && end <= other.end()) {
			const size_t left_len = static_cast<char*>(end) - static_cast<char*>(other.start);

			if(left_len == other.len) {
				//  Update the existing region, no need to split
				other.type = type;
			} else {
				//  Create a region from <existing.start, new.end) of new.type
				create_new_region(other.start, left_len, type);
				//  Move the existing region
				other.start = end;
				other.len -= left_len;
			}

			//  There may also be a range that goes outside the currently checked
			//  region. In this case, adjust the region length and restart the whole
			//  process again. We can't simply create a new region, as the OOB range
			//  may actually be overlapping a different one.
			if(len > left_len) {
				len -= left_len;
				continue;
			}
		}
		//  2)
		else if(start >= other.start && end > other.end()) {
			auto* existing_end = other.end();
			const size_t right_len = static_cast<char*>(existing_end) - static_cast<char*>(start);

			if(right_len == other.len) {
				//  Update the existing region, no need to split
				other.type = type;
			} else {
				//  Create a region from <existing.start, new.start) of existing.type
				create_new_region(other.start, other.len - right_len, other.type);
				//  Move, shrink and change the type of the existing region to the new
				//  one. Intentionally do it this way so that the previous region is
				//  easily accessible at s_regions[current-1].
				other.start = start;
				other.len = right_len;
				other.type = type;
			}

			//  There may also be a range that goes outside the currently checked
			//  region. In this case, adjust the region start and length and restart
			//  the whole process again. We can't simply create a new region, as the
			//  OOB range may actually be overlapping a different one.
			if(len > right_len) {
				len -= right_len;
				start = existing_end;
				continue;
			}
		}
		//  3)
		else if(start > other.start && end < other.end()) {
			const auto left_len = static_cast<char*>(start) - static_cast<char*>(other.start);
			const auto right_len = static_cast<char*>(other.end()) - static_cast<char*>(end);

			//  Create a region for <existing.start,new.start) of existing.type
			create_new_region(other.start, left_len, other.type);
			//  Create a region for <new.end,existing.end) of existing.type
			create_new_region(end, right_len, other.type);
			//  Shrink, move and change the type of the existing region
			other.start = start;
			other.len = other.len - left_len - right_len;
			other.type = type;
		}
		//  5)
		else {
			if(current == 0) {
				//  Insert a hole region
				const auto reg_len = static_cast<char*>(other.start) - static_cast<char*>(start);
				create_new_region(start, reg_len, type);
				//  Move the current pointer
				start = other.start;
				len -= reg_len;
			} else {
				auto& previous = s_regions[current - 1];
				const auto reg_len = static_cast<char*>(other.start) - static_cast<char*>(previous.end());
				//  In default state, the region list is sorted. Modifications during
				//  region update obviously break this sorting. However, what we really
				//  require is that the previous region (at index `current - 1`)
				//  corresponds to the region that is right before the current one in
				//  memory, and only for the purposes of determining the hole between two
				//  consecutive regions. The regions will go from:
				//    [previous] ... hole ... [current]
				//  to:
				//    [previous] [new_region] [current]
				//
				//  Recreate the previous region. This puts the region at the end of
				//  the array, but it allows us to re-use its slot to store the hole
				//  region.
				create_new_region(previous.start, previous.len, previous.type);
				//  Create the new hole region. This spans from the end of the previous
				//  region to the start of the current one. The type is set based on the
				//  requested type.
				previous.start = previous.end();
				previous.len = reg_len;
				previous.type = type;
				//  Move the current pointer to the next location
				start = other.start;
				len -= reg_len;
			}
			//  We're not done handling the current region yet, we've only
			//  added the hole region that is necessary to turn the 5) scenario
			//  into a 2) (where both start addresses are equal), so we evaluate
			//  the same logic as if nothing happened.
			goto evilgotogoesbrr;
		}

		goto sort;
	}

	//  No overlaps - simply create a new region
	create_new_region(start, len, type);

sort:
	sort_regions(s_regions, s_region_count);
}

tinymm::Region const* tinymm::regions(size_t* count) {
	if(!count) {
		return nullptr;
	}

	*count = s_region_count;
	return s_regions;
}
