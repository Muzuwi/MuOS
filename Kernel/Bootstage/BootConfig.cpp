#include <Arch/i386/BootConfig.hpp>
#include <Arch/i386/Multiboot.hpp>
#include <Kernel/Memory/VirtualMemManager.hpp>
#include <Kernel/Debug/kdebugf.hpp>

extern uint32_t _ukernel_virtual_offset;

/*
	This function takes a pointer from a multiboot structure
	and makes sure that memory referenced by it is mapped, and
	returns a pointer to it in the kernel address space
*/
uint32_t* sanitize_multiboot_pointer(uint32_t* pointer) {
	if(!pointer) {
		return nullptr;
	}

	uint32_t* virtual_address = (uint32_t*)((uint32_t)pointer + (uint32_t)&_ukernel_virtual_offset); 

	return virtual_address;

//	if(!VMM::get_directory()->get_page(virtual_address)) {
//		Paging::allocate_page((void*)pointer, (void*)virtual_address);
//	}

//	kdebugf("[multiboot_sanitizer] Sanitized %x to %x\n", (uint32_t)pointer, (uint32_t)virtual_address);

//	return virtual_address;
}


void BootConfig::parse_multiboot_structure(uintptr_t* multiboot_info){
	//  FIXME:  Right now this function is a horrible mess, refactor
	return;

	uint32_t* phys_multiboot_info = sanitize_multiboot_pointer(multiboot_info);

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
		uint32_t* address = sanitize_multiboot_pointer((uint32_t*)phys_multiboot_info[4]);

		if(address) {
			kdebugf("kernel arguments: %s\n", address);
		}
	}

	if(flags & multiboot_flag_t::MULTIBOOT_MODULES)
		kdebugf("module count: %i\n", phys_multiboot_info[5]);

	if(flags & 16 || flags & 32)
		kdebugf("flags 4 or 5 present\n");

	if(flags & multiboot_flag_t::MULTIBOOT_MEMMAP)
		VMM::get().parse_multiboot_mmap(&phys_multiboot_info[11]);

	if(flags & multiboot_flag_t::MULTIBOOT_BOOTNAME){
		uint32_t* address = sanitize_multiboot_pointer((uint32_t*)phys_multiboot_info[16]);
		if(address) {
			kdebugf("bootloader name: %s\n", address);
		}
	}
}
