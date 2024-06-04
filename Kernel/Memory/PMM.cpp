#include <Arch/x86_64/Boot/MultibootInfo.hpp>
#include <Arch/x86_64/LinkscriptSyms.hpp>
#include <Core/Log/Logger.hpp>
#include <Core/MP/MP.hpp>
#include <Memory/KHeap.hpp>
#include <Memory/PMM.hpp>
#include <Memory/Units.hpp>
#include <Process/Thread.hpp>

PMM PMM::s_instance {};
CREATE_LOGGER("pmm", core::log::LogLevel::Debug);

void PMM::create_normal_region(void* start, size_t size) {
	auto region = PRegion { PhysAddr { start }, size };
	if(!region.allocator().deferred_initialization()) {
		region.allocator().initialize();
	}
	m_normal_regions.push_back(region);
}

void PMM::create_lowmem_region(void* start, size_t size) {
	auto region = PRegion { PhysAddr { start }, size };
	if(!region.allocator().deferred_initialization()) {
		region.allocator().initialize();
	}
	m_mem16_regions.push_back(region);
}

void PMM::init_regions(PhysPtr<MultibootInfo> multiboot_info) {
	log.info("Multiboot memory map:");

	auto mmap_addr = multiboot_info->mmap();
	auto mmap_end = multiboot_info->mmap_end();

	//  Total system memory
	uint64_t memory_amount = 0, reserved_amount = 0;

	auto type_string = [](MultibootMMap::RegionType type) -> char const* {
		switch(type) {
			case MultibootMMap::RegionType::USABLE: return "usable";
			case MultibootMMap::RegionType::HIBERN: return "to be preserved";
			case MultibootMMap::RegionType::ACPI: return "acpi";
			case MultibootMMap::RegionType::BAD: return "defective";
			default: return "reserved";
		}
	};

	auto pointer = mmap_addr.get_mapped();
	while(pointer < mmap_end.get_mapped()) {
		auto start = pointer->start();
		auto range = pointer->range();
		auto end = start + range;

		log.info("|- {x} - {x}: {}", start, end, type_string(pointer->type()));

		if((uintptr_t)m_physical_end.get() < end) {
			m_physical_end = PhysAddr { (void*)end };
		}

		switch(pointer->type()) {
			case MultibootMMap::RegionType::USABLE: {
				memory_amount += range;

				//  Add lowram as a separate allocator type
				if(end < 1 * Units::MiB) {
					//  Push back the start for zero page, as it contains the BIOS IVT,
					//  which is used in V86 mode
					if(start == 0x0) {
						start = 0x1000;
						range -= 0x1000;
					}
					create_lowmem_region((void*)start, range);
					break;
				}

				auto kernel_phys_start = (uint64_t)&_ukernel_preloader_physical;
				auto kernel_phys_end = (uint64_t)&_ukernel_elf_end - (uint64_t)&_ukernel_virtual_offset;

				ENSURE((kernel_phys_start & 0xFFF) == 0);
				ENSURE((kernel_phys_end & 0xFFF) == 0);

				//  Split regions overlapping with the kernel executable
				if(start < kernel_phys_end) {
					if(end > kernel_phys_end) {
						if(start < kernel_phys_start) {
							create_normal_region((void*)start, kernel_phys_start - start);
						}
						create_normal_region((void*)kernel_phys_end, end - kernel_phys_end);
					}
				} else {
					create_normal_region((void*)start, range);
				}

				break;
			}
			case MultibootMMap::RegionType::HIBERN: {
				//  explicit empty case
				break;
			}
			case MultibootMMap::RegionType::ACPI: {
				//  explicit empty case
				break;
			}
			case MultibootMMap::RegionType::BAD: {
				//  explicit empty case
				break;
			}
			default: {
				reserved_amount += range;
				break;
			}
		}

		pointer = pointer->next_entry();
	}

	auto kernel_start = (uint64_t)(&_ukernel_elf_start), kernel_end = (uint64_t)(&_ukernel_elf_end);

	uint32_t mem_mib = memory_amount / 0x100000;

	log.info("Kernel-used memory: {} KiB", (kernel_end - kernel_start) / 0x1000);
	log.info("Total usable memory: {} MiB", mem_mib);
	log.info("Reserved memory: {} bytes", reserved_amount);
	log.info("Physical end: {}", m_physical_end.get());
}

void PMM::init_deferred_allocators() {
	for(auto& region : m_normal_regions) {
		if(region.allocator().deferred_initialization()) {
			region.allocator().initialize();
		}
	}

	log.info("Regions initialized:");
	for(auto& reg : m_mem16_regions) {
		auto start = (uint64_t)reg.base().get();
		log.info("|- PRegion[low]: start {x}, size {}", start, reg.size());
	}
	for(auto& reg : m_normal_regions) {
		auto start = (uint64_t)reg.base().get();
		log.info("|- PRegion: start {x}, size {}", start, reg.size());
	}
}

/*
 *  Normal pool regions
 */
[[nodiscard]] KOptional<PAllocation> PMM::allocate(size_t count_order) {
	auto* thread = this_cpu()->current_thread();

	if(thread) {
		thread->preempt_disable();
	}
	m_pmm_lock.lock();
	auto result = [this, count_order]() -> auto {
		for(auto& region : m_normal_regions) {
			auto ret = region.allocator().allocate(count_order);
			if(ret.has_value()) {
				auto page = ret.unwrap();
				auto allocation = PAllocation(page, count_order);
				return KOptional<PAllocation> { allocation };
			}
		}
		return KOptional<PAllocation> {};
	}();
	m_pmm_lock.unlock();
	if(thread) {
		thread->preempt_enable();
	}

	return result;
}

void PMM::free(PAllocation const& allocation) {
	auto* thread = this_cpu()->current_thread();

	if(thread) {
		thread->preempt_disable();
	}
	m_pmm_lock.lock();
	for(auto& region : m_normal_regions) {
		if(region.contains(allocation.base()) && region.contains(allocation.end())) {
			region.allocator().free(allocation.base(), allocation.order());
			break;
		}
	}
	m_pmm_lock.unlock();
	if(thread) {
		thread->preempt_enable();
	}
}

/*
 *  Lowmem pool regions
 */
[[nodiscard]] KOptional<PhysAddr> PMM::allocate_lowmem() {
	auto* thread = this_cpu()->current_thread();

	if(thread) {
		thread->preempt_disable();
	}
	m_pmm_lock.lock();
	auto result = [this]() -> auto {
		for(auto& region : m_mem16_regions) {
			auto ret = region.allocator().allocate(0);
			if(ret.has_value()) {
				auto page = ret.unwrap();
				return KOptional<PhysAddr> { page };
			}
		}
		return KOptional<PhysAddr> {};
	}();
	m_pmm_lock.unlock();
	if(thread) {
		thread->preempt_enable();
	}

	return result;
}

void PMM::free_lowmem(PhysAddr addr) {
	auto* thread = this_cpu()->current_thread();

	if(thread) {
		thread->preempt_disable();
	}
	m_pmm_lock.lock();
	for(auto& region : m_mem16_regions) {
		if(region.contains(addr)) {
			region.allocator().free(addr, 0);
			break;
		}
	}
	m_pmm_lock.unlock();
	if(thread) {
		thread->preempt_enable();
	}
}

PhysAddr PMM::physical_end_addr() {
	return m_physical_end;
}
