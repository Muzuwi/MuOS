#include <arch/i386/BootConfig.hpp>
#include <arch/i386/MemManager.hpp>
#include <arch/i386/multiboot.hpp>
#include <kernel/kdebugf.hpp>

extern uint32_t _ukernel_virtual_offset;

void BootConfig::parse_multiboot_structure(uintptr_t* multiboot_info){
	//  At this point, the first 1MiB is not 1:1 mapped anymore, we need to 
	//  add the virtual offset to get the virtual address to it
	uint32_t* phys_multiboot_info = (uint32_t*)((uintptr_t)multiboot_info + (uintptr_t)&_ukernel_virtual_offset);

	if(!phys_multiboot_info){
		kerrorf("[BootConfig] No valid multiboot structure passed, panic!\n");
		asm volatile("cli\nhlt");
	}
	kdebugf("[BootConfig] Multiboot structure at %x\n", phys_multiboot_info);

	auto flags = (uint32_t)phys_multiboot_info[0];
	kdebugf("flags: %x\n", flags);

	if(flags & multiboot_flag_t::MULTIBOOT_MEMINFO)
		kdebugf("mem lower: %x, mem upper: %x\n", phys_multiboot_info[1], phys_multiboot_info[2]);

	if(flags & multiboot_flag_t::MULTIBOOT_BOOTDEVICE)
		kdebugf("boot device: %x\n", phys_multiboot_info[3]);

	if(flags & multiboot_flag_t::MULTIBOOT_CMDLINE){
		uint32_t* address = (uint32_t*)(phys_multiboot_info[4] + (uint32_t)&_ukernel_virtual_offset);
		kdebugf("kernel arguments: %s\n", address);
	}

	if(flags & multiboot_flag_t::MULTIBOOT_MODULES)
		kdebugf("module count: %i\n", phys_multiboot_info[5]);

	if(flags & 16 || flags & 32)
		kdebugf("flags 4 or 5 present\n");

	if(flags & multiboot_flag_t::MULTIBOOT_MEMMAP)
		MemManager::get().parse_multiboot_mmap(&phys_multiboot_info[11]);

	if(flags & multiboot_flag_t::MULTIBOOT_BOOTNAME){
		uint32_t* address = (uint32_t*)(phys_multiboot_info[16] + (uint32_t)&_ukernel_virtual_offset);
		kdebugf("bootloader name: %s\n", address);
	}
}