#include <Kernel/Debug/kdebugf.hpp>
#include <Kernel/Memory/KHeap.hpp>
#include <Kernel/Memory/kmalloc.hpp>
#include <Kernel/Memory/PMM.hpp>
#include <Kernel/Memory/PRegion.hpp>
#include <Kernel/Multiboot/MultibootInfo.hpp>
#include <Kernel/Symbols.hpp>
#include <LibGeneric/Vector.hpp>

using gen::Vector;
using Units::MiB;

static Vector<PRegion, KMalloc::BootstrapAllocator> s_available_regions {};
static Vector<PRegion, KMalloc::BootstrapAllocator> s_defered_regions {};

static void pmm_handle_new_region(PhysAddr base_address, size_t region_size) {
//	kdebugf("Creating region for %x%x - size %i\n", (uintptr_t)base_address.get() >> 32u, (uintptr_t)base_address.get() & 0xFFFFFFFFu, region_size);
	auto region = PRegion{base_address, region_size};

	if(region.allocator().deferred_initialization())
		s_defered_regions.push_back(region);
	else
		s_available_regions.push_back(region);
}

void PMM::handle_multiboot_memmap(PhysPtr<MultibootInfo> multiboot_info) {
	kdebugf("[PMM] Multiboot memory map:\n");

	auto mmap_addr = multiboot_info->mmap();
	auto mmap_end = multiboot_info->mmap_end();

	//  Total system memory
	uint64_t memory_amount = 0, reserved_amount = 0;

	auto pointer = mmap_addr.get_mapped();
	while (pointer < mmap_end.get_mapped()) {
		auto start = pointer->start();
		auto range = pointer->range();
		auto end = start + range;

		kdebugf("[PMM] %x%x - %x%x: ", start >> 32u, start & 0xFFFFFFFFu, end >> 32u, end & 0xFFFFFFFFu);

		switch (pointer->type()) {
			case MultibootMMap::RegionType::USABLE: {
				auto pages = range / 0x1000;
				auto required_bitmap_pages = (pages / (0x1000 * 8));
				kdebugf("usable, pages: %i, buf: %i\n", pages, required_bitmap_pages);
				memory_amount += range;

				//  Avoid low RAM, to not trample over the bootstrap memory
				if (end < 1 * MiB)
					break;

				auto kernel_phys_start = (uint64_t)&_ukernel_preloader_physical;
				auto kernel_phys_end   = (uint64_t)&_ukernel_elf_end-(uint64_t)&_ukernel_virtual_offset;

				//  Split regions overlapping with the kernel executable
				if(start < kernel_phys_end) {
			        if(end > kernel_phys_end) {
			        	if(start < kernel_phys_start)
					        pmm_handle_new_region(PhysAddr{(void*)start}, kernel_phys_start - start);
				        pmm_handle_new_region(PhysAddr{(void*)kernel_phys_end}, end - kernel_phys_end);
			        }
				} else {
					pmm_handle_new_region(PhysAddr{(void*)start}, range);
				}

				break;
			}
			case MultibootMMap::RegionType::HIBERN:
				kdebugf("to be preserved\n");
				break;

			case MultibootMMap::RegionType::ACPI:
				kdebugf("acpi\n");
				break;

			case MultibootMMap::RegionType::BAD:
				kdebugf("defective\n");
				break;

			default:
				kdebugf("reserved\n");
				reserved_amount += range;
				break;
		}

		pointer = pointer->next_entry();
	}

	auto kernel_start = (uint64_t) (&_ukernel_elf_start),
			kernel_end = (uint64_t) (&_ukernel_elf_end);

	uint32_t mem_mib = memory_amount / 0x100000;

	kdebugf("[PMM] Kernel-used memory: %i KiB\n", (kernel_end - kernel_start) / 0x1000);
	kdebugf("[PMM] Total usable memory: %i MiB\n", mem_mib);
	kdebugf("[PMM] Reserved memory: %i bytes\n", reserved_amount);
}

[[nodiscard]] KOptional<PAllocation> PMM::allocate(size_t count_order) {
	for(auto& region : s_available_regions) {
		auto ret = region.allocator().allocate(count_order);
		if(ret.has_value()) {
			auto page = ret.unwrap();
			auto allocation = PAllocation(page, count_order);

			return KOptional<PAllocation>{allocation};
		}
	}

	kerrorf("[PMM] Allocation failure for order of page_count=%i\n", (1u<<count_order));
	return KOptional<PAllocation>{};
}

void PMM::free_allocation(const PAllocation& allocation) {
	kdebugf("[PMM] Deallocate PAlloc base=%x%x, order=%i\n", (uint64_t)allocation.base().get() >> 32u, (uint64_t)allocation.base().get() & 0xffffffffu, allocation.order());

	for(auto& region : s_available_regions) {
		if(region.contains(allocation.base()) && region.contains(allocation.end())) {
			region.allocator().free(allocation.base(), allocation.order());
			return;
		}
	}

	kerrorf("[PMM] Deallocation failure\n");
}

void PMM::initialize_deferred_regions() {
	while (!s_defered_regions.empty()) {
		auto region = s_defered_regions.pop_back();
		region.allocator().initialize();
		s_available_regions.push_back(region);
	}

	kdebugf("[PMM] Regions initialized:\n");
	for(auto& reg : s_available_regions) {
		auto start = (uint64_t)reg.base().get();
		kdebugf("  - PRegion: start %x%x, size %i\n", start >> 32u, start & 0xffffffffu, reg.size());
	}
}
