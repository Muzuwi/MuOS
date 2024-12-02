#pragma once
#include <SystemTypes.hpp>

//  How many unique physical memory regions can be handled by tinymm,
//  regardless of if they're used or not.
#define TINYMM_MAX_REGIONS (32)

namespace tinymm {
	enum class RegionType {
		Usable = 0,
		ReservedByHardware,
		Allocator
	};

	struct Region {
		void* start;
		size_t len;
		tinymm::RegionType type;

		[[nodiscard]] inline void* end() const { return reinterpret_cast<void*>(static_cast<char*>(start) + len); }
	};

	void update_region(void* start, size_t len, RegionType type);
	Region const* regions(size_t* count);
}