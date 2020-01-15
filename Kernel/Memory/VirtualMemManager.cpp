#include <Kernel/Memory/VirtualMemManager.hpp>
#include <Arch/i386/multiboot.hpp>
#include <Kernel/Debug/kdebugf.hpp>

extern uint32_t _ukernel_start, _ukernel_end, _ukernel_virtual_start, _ukernel_virtual_offset;

VirtualMemManager::VirtualMemManager() {

}

VirtualMemManager& VirtualMemManager::get(){
	static VirtualMemManager vmemmanager;
	return vmemmanager;
}

void VirtualMemManager::parse_multiboot_mmap(uintptr_t* multiboot_mmap) {
	auto mmap_len = multiboot_mmap[0];
	auto mmap_addr = (uint32_t)multiboot_mmap[1] + (uint32_t)&_ukernel_virtual_offset;
	kdebugf("[VMM] mmap size: %x\n", mmap_len);
	kdebugf("[VMM] mmap addr: %x\n", mmap_addr);

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

	uint32_t mem_mib = memory_amount / 0x100000;

	kdebugf("[VMM] Kernel-used memory: %i MiB\n", (kernel_end - kernel_start) / 0x100000);
	kdebugf("[VMM] Total usable memory: %i MiB\n", mem_mib);
	kdebugf("[VMM] Reserved memory: %i bytes\n", reserved_amount);
}
