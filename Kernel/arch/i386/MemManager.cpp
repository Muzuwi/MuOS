#include <arch/i386/MemManager.hpp>
#include <arch/i386/multiboot.hpp>
#include <kernel/kdebugf.hpp>
#define assert(a)

extern uint32_t _ukernel_start, _ukernel_end;
extern uint32_t _ukernel_virtual_offset;

static MemManager manager;
MemManager* MemManager::instance = nullptr;

MemManager& MemManager::get() {
	//  TODO: Asserts
	assert(MemManager::instance);
	return (MemManager&)MemManager::instance;
}

MemManager::MemManager(){
	if(instance){
		kerrorf("Only one instance of MemManager allowed!\n");
		asm volatile("cli\nhlt");
	}
	instance = this;
	m_free_mem_ranges_count = 0;
	return;
}

/*
	Parses the multiboot-provided memory map
	and stores the ranges
*/
void MemManager::parse_multiboot_mmap(uintptr_t* multiboot_mmap) {
	auto mmap_len = multiboot_mmap[0];
	auto mmap_addr = (uint32_t)multiboot_mmap[1] + (uint32_t)&_ukernel_virtual_offset;
	kdebugf("mmap size: %x\n", mmap_len);
	kdebugf("mmap addr: %x\n", mmap_addr);

	//  Total system memory
	uint64_t memory_amount = 0, reserved_amount = 0;

	uint32_t pointer = mmap_addr;
	while(pointer < mmap_addr + mmap_len){
		uint32_t size  = *((uint32_t*)pointer);
		uint64_t start = *((uint64_t*)(pointer + 4));
		uint64_t range = *((uint64_t*)(pointer + 12)); 
		uint64_t end   = start + range;
		auto type = *((uint32_t*)(pointer + 20));
		kdebugf("%x%x", (uint32_t)((start >> 32) & (0xFFFFFFFF)),(uint32_t)(start & 0xFFFFFFFF));			
		kdebugf(" - %x%x: ", (uint32_t)((end >> 32) & (0xFFFFFFFF)),(uint32_t)(end & 0xFFFFFFFF));

		switch((mmap_memory_type_t)type){
			case USABLE:
				kdebugf("usable\n");
				memory_amount += range;

				m_free_mem_ranges[m_free_mem_ranges_count++] = mem_range_t(start, end);
				break;

			case HIBERN:
				kdebugf("to be preserved\n");
				break;

			case ACPI:
				kdebugf("acpi\n");
				break;

			case BAD:
				kdebugf("defective\n");
				break;

			default:
				kdebugf("reserved\n");
				reserved_amount += range;
				break;
		}
		
		pointer += size + 4;
	}

	uint32_t kernel_start = (uint32_t)(&_ukernel_start),
			 kernel_end   = (uint32_t)(&_ukernel_end);

	kdebugf("Free memory ranges [%x]: \n", (int)m_free_mem_ranges_count);
	for(size_t i = 0; i < m_free_mem_ranges_count; i++){
		auto start = m_free_mem_ranges[i].m_start,
			 end   = m_free_mem_ranges[i].m_end;
		kdebugf("%x%x", (uint32_t)((start >> 32) & (0xFFFFFFFF)),(uint32_t)(start & 0xFFFFFFFF));			
		kdebugf(" - %x%x\n", (uint32_t)((end >> 32) & (0xFFFFFFFF)),(uint32_t)(end & 0xFFFFFFFF));
		// if(kernel_end < end) {
		// 	if(((kernel_end & 0xFFF00000) + 0x100000) < end){
		// 		kdebugf("potential userland start: %x\n", ((kernel_end & 0xFFF00000) + 0x100000));
		// 	}
		// }
	}

	uint32_t mem_mib = memory_amount / 0x100000;

	kdebugf("Kernel-used memory: %i MiB\n", (kernel_end - kernel_start) / 0x100000);
	kdebugf("Total usable memory: %i MiB\n", mem_mib);
	kdebugf("Reserved memory: %i bytes\n", reserved_amount);

}

/*
	Returns the amount of bytes free in current memory ranges
	TODO:
*/
uint64_t MemManager::get_free() {

	return (uint64_t)0;
}

/*
	TODO:
*/
void* MemManager::allocate_kernel(size_t size) {

}

