#include <arch/i386/BootConfig.hpp>
#include <arch/i386/MemManager.hpp>
#include <arch/i386/multiboot.hpp>
#include <kernel/kdebugf.hpp>


void BootConfig::parse_multiboot_structure(uintptr_t* multiboot_info){
	if(!multiboot_info){
		kerrorf("[BootConfig] No valid multiboot structure passed, panic!\n");
		asm volatile("cli\nhlt");
	}
	kdebugf("[BootConfig] Multiboot structure at %x\n", multiboot_info);

	auto flags = (uint32_t)multiboot_info[0];
	kdebugf("flags: %x\n", flags);

	if(flags & multiboot_flag_t::MULTIBOOT_MEMINFO)
		kdebugf("mem lower: %x, mem upper: %x\n", multiboot_info[1], multiboot_info[2]);

	if(flags & multiboot_flag_t::MULTIBOOT_BOOTDEVICE)
		kdebugf("boot device: %x\n", multiboot_info[3]);

	if(flags & multiboot_flag_t::MULTIBOOT_CMDLINE)
		kdebugf("kernel arguments: %s\n", (uintptr_t*)multiboot_info[4]);

	if(flags & multiboot_flag_t::MULTIBOOT_MODULES)
		kdebugf("module count: %i\n", multiboot_info[5]);

	if(flags & 16 || flags & 32)
		kdebugf("flags 4 or 5 present\n");

	if(flags & multiboot_flag_t::MULTIBOOT_MEMMAP)
		MemManager::get().parse_multiboot_mmap(&multiboot_info[11]);

	if(flags & multiboot_flag_t::MULTIBOOT_BOOTNAME)
		kdebugf("bootloader name: %s\n", (uintptr_t*)multiboot_info[16]);
}