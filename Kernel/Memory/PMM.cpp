#include <Debug/klogf.hpp>
#include <Kernel/Memory/KHeap.hpp>
#include <Kernel/Memory/kmalloc.hpp>
#include <Kernel/Memory/PMM.hpp>
#include <Kernel/Memory/PRegion.hpp>
#include <Kernel/Multiboot/MultibootInfo.hpp>
#include <Kernel/Symbols.hpp>

PMM PMM::s_instance {};

void PMM::init_regions(PhysPtr<MultibootInfo> multiboot_info) {
	klogf_static("[PMM] Multiboot memory map:\n");

	auto mmap_addr = multiboot_info->mmap();
	auto mmap_end = multiboot_info->mmap_end();

	//  Total system memory
	uint64_t memory_amount = 0, reserved_amount = 0;

	auto pointer = mmap_addr.get_mapped();
	while(pointer < mmap_end.get_mapped()) {
		auto start = pointer->start();
		auto range = pointer->range();
		auto end = start + range;

		klogf_static("[PMM] {x} - {x}: ", start, end);

		switch(pointer->type()) {
			case MultibootMMap::RegionType::USABLE: {
				auto pages = range / 0x1000;
				auto required_bitmap_pages = (pages / (0x1000 * 8));
				klogf_static("usable, pages: {}, buf: {}\n", pages, required_bitmap_pages);
				memory_amount += range;

				//  Add lowram as a separate allocator type
				if(end < 1 * Units::MiB) {
					//  Push back the start for zero page, as it contains the BIOS IVT,
					//  which is used in V86 mode
					if(start == 0x0) {
						start = 0x1000;
						range -= 0x1000;
					}
					m_mem16_regions.push_back({PhysAddr {(void*)start}, range});
					break;
				}

				auto kernel_phys_start = (uint64_t)&_ukernel_preloader_physical;
				auto kernel_phys_end = (uint64_t)&_ukernel_elf_end - (uint64_t)&_ukernel_virtual_offset;

				//  Split regions overlapping with the kernel executable
				if(start < kernel_phys_end) {
					if(end > kernel_phys_end) {
						if(start < kernel_phys_start) {
							m_normal_regions.push_back({PhysAddr {(void*)start}, kernel_phys_start - start});
						}
						m_normal_regions.push_back({PhysAddr {(void*)kernel_phys_end}, end - kernel_phys_end});
					}
				} else {
					m_normal_regions.push_back({PhysAddr {(void*)start}, range});
				}

				break;
			}
			case MultibootMMap::RegionType::HIBERN: {
				klogf_static("to be preserved\n");
				break;
			}
			case MultibootMMap::RegionType::ACPI: {
				klogf_static("acpi\n");
				break;
			}
			case MultibootMMap::RegionType::BAD: {
				klogf_static("defective\n");
				break;
			}
			default: {
				klogf_static("reserved\n");
				reserved_amount += range;
				break;
			}
		}

		pointer = pointer->next_entry();
	}

	auto kernel_start = (uint64_t)(&_ukernel_elf_start),
			kernel_end = (uint64_t)(&_ukernel_elf_end);

	uint32_t mem_mib = memory_amount / 0x100000;

	klogf_static("[PMM] Kernel-used memory: {} KiB\n", (kernel_end - kernel_start) / 0x1000);
	klogf_static("[PMM] Total usable memory: {} MiB\n", mem_mib);
	klogf_static("[PMM] Reserved memory: {} bytes\n", reserved_amount);
}

void PMM::init_deferred_allocators() {
	for(auto& region : m_normal_regions) {
		if(region.allocator().deferred_initialization()) {
			region.allocator().initialize();
		}
	}

	klogf_static("[PMM] Regions initialized:\n");
	for(auto& reg : m_mem16_regions) {
		auto start = (uint64_t)reg.base().get();
		klogf_static("  - PRegion[low]: start {x}, size {}\n", start, reg.size());
	}
	for(auto& reg : m_normal_regions) {
		auto start = (uint64_t)reg.base().get();
		klogf_static("  - PRegion: start {x}, size {}\n", start, reg.size());
	}
}


/*
 *  Normal pool regions
 */
[[nodiscard]] KOptional<PAllocation> PMM::allocate(size_t count_order) {
	for(auto& region : m_normal_regions) {
		auto ret = region.allocator().allocate(count_order);
		if(ret.has_value()) {
			auto page = ret.unwrap();
			auto allocation = PAllocation(page, count_order);
			return KOptional<PAllocation> {allocation};
		}
	}

	klogf_static("[PMM] Allocation failure for order of page_count={}\n", (1u << count_order));
	return KOptional<PAllocation> {};
}

void PMM::free(PAllocation const& allocation) {
	for(auto& region : m_normal_regions) {
		if(region.contains(allocation.base()) && region.contains(allocation.end())) {
			region.allocator().free(allocation.base(), allocation.order());
			return;
		}
	}

	klogf_static("[PMM] Deallocation failure\n");
}

/*
 *  Lowmem pool regions
 */
KOptional<PhysAddr> PMM::allocate_lowmem() {
	for(auto& region : m_mem16_regions) {
		auto ret = region.allocator().allocate(0);
		if(ret.has_value()) {
			auto page = ret.unwrap();
			return KOptional<PhysAddr> {page};
		}
	}

	return KOptional<PhysAddr> {};
}

void PMM::free_lowmem(PhysAddr addr) {
	for(auto& region : m_mem16_regions) {
		if(region.contains(addr)) {
			region.allocator().free(addr, 0);
			return;
		}
	}

	klogf_static("[PMM] Failed freeing lowmem at {}\n", Format::ptr(addr.get()));
}
