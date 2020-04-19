#include <Kernel/Memory/VirtualMemManager.hpp>
#include <Arch/i386/Multiboot.hpp>
#include <Kernel/Debug/kdebugf.hpp>
#include <Arch/i386/PageDirectory.hpp>
#include <Kernel/Debug/kpanic.hpp>

extern uint32_t _ukernel_start, _ukernel_end, _ukernel_virtual_start;

uint32_t s_kernel_directory_table[1024] __attribute__((aligned(4096)));
PageDirectory* VMM::s_kernel_directory = reinterpret_cast<PageDirectory*>(&s_kernel_directory_table[0]);

/*
 *	Initializes the memory manager of the kernel
 *  TODO: Set permissions on the kernel executable sections
 */
void VMM::init() {
	kdebugf("[VMM] Removing identity mappings\n");

	//  FIXME:  Add a method to PageDirectory that allows unmapping entire tables
	//  and actually flushes them afterwards
	s_kernel_directory->get_entry((uint32_t*)0x0).set_flag(DirectoryFlag::Present, false);
}

/*
 *	Returns the kernel page directory
 */
PageDirectory* VMM::get_directory() {
	return s_kernel_directory;
}

VMM& VMM::get(){
	static VMM virtual_memory_manager;
	return virtual_memory_manager;
}



/*
 *	Maps a page of memory at 'phys_addr' to virtual address 'virt_addr'
 */
void VMM::map(uintptr_t *virt_addr, uintptr_t *phys_addr) {
//	kdebugf("[vmm] map %x -> %x\n", (unsigned)virt_addr, (unsigned)phys_addr);
	auto& dir = s_kernel_directory->get_entry(virt_addr);
	auto* table = dir.get_table();

	if(!table) {
		kdebugf("[vmm] table for %x does not exist\n", (unsigned)virt_addr);
		s_kernel_directory->create_table(virt_addr);
		table = dir.get_table();
		if(!table) kpanic();
	}


	auto& page = table->get_page(virt_addr);

	if(!dir.get_flag(DirectoryFlag::Present)) dir.set_flag(DirectoryFlag::Present, true);
	dir.set_flag(DirectoryFlag::RW, true);

	page.set_physical(phys_addr);
	page.set_flag(PageFlag::Present, true);
	page.set_flag(PageFlag::Global, true);
	page.set_flag(PageFlag::RW, true);

	invlpg(virt_addr);
}

/*
 *	Unmaps a page of memory previously mapped at virt_addr
 *	FIXME: Untested, adding in for completeness, not used currently and will most likely get rewritten
 */
void VMM::unmap(uintptr_t *virt_addr) {
	auto dir = s_kernel_directory->get_entry(virt_addr);
	auto table = dir.get_table();

	//  Nonexistent address
	if(!table) return;

	auto page = table->get_page(virt_addr);

	page.set_flag(PageFlag::Present, false);
	invlpg(virt_addr);
}
