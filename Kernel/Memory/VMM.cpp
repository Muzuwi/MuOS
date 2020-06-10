#include <Kernel/Memory/VMM.hpp>
#include <Arch/i386/Multiboot.hpp>
#include <Kernel/Debug/kdebugf.hpp>
#include <Arch/i386/PageDirectory.hpp>
#include <Kernel/Debug/kpanic.hpp>
#include <Kernel/Symbols.hpp>
#include <Kernel/Process/Process.hpp>
#include <Kernel/Memory/VMapping.hpp>
#include <Arch/i386/IRQDisabler.hpp>
#include <include/Kernel/Memory/kmalloc.hpp>
#include <Kernel/Memory/QuickMap.hpp>
#include <string.h>

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

	kdebugf("[VMM] Locking write on read-only regions\n");
	uintptr_t cur = (uintptr_t)&_ukernel_RO_begin;
	while(cur < (uintptr_t)&_ukernel_RO_end) {
		auto* page = s_kernel_directory->get_page((uint32_t*)cur);
		if(page) {
			page->set_flag(PageFlag::RW, false);
			invlpg((uintptr_t*)cur);
		}

		cur += 4096;
	}

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

/*
 *  Allocates stack space with given size for the current process
 */
void* VMM::allocate_user_stack(size_t stack_size) {
	//  Process stacks are allocated right before the kernel virtual address space
	auto* stack_top = (void*)((uint32_t)&_ukernel_virtual_offset-stack_size);
	auto* mapping = new VMapping(stack_top, stack_size, PROT_READ | PROT_WRITE, MAP_SHARED);
	auto* process = Process::m_current;

	process->m_maps.push_back(mapping);

	return (void*)((uint32_t)&_ukernel_virtual_offset - 1);
}

void VMM::notify_create_VMapping(VMapping& mapping) {
#ifdef VMM_LOG_VMAPPING
	kdebugf("[VMM] Created VMapping(%x)\n", &mapping);
#endif

	auto* process = Process::m_current;
	auto* dir = process->m_directory;
	QuickMap mapper{dir};

	if((uint64_t)dir < (uint64_t)&_ukernel_virtual_offset) {
		dir = reinterpret_cast<PageDirectory*>(mapper.address());
	}

	auto current_virt_addr = mapping.addr();

	for(auto& map : mapping.pages()) {
#ifdef VMM_LOG_VMAPPING
		kdebugf("Virt: %x -> Phys %x\n", current_virt_addr, map->address());
#endif

		auto& pde = dir->get_entry((uint32_t*)current_virt_addr);
		auto& table = VMM::ensure_page_table(pde);
		auto& page = table.get_page((uint32_t*)current_virt_addr);

		pde.set_flag(DirectoryFlag::Present, true);
		pde.set_flag(DirectoryFlag::User, process->m_ring == Ring::CPL3);
		pde.set_flag(DirectoryFlag::RW, (mapping.flags() & PROT_WRITE));

		page.set_physical((uintptr_t*)(map->address()));
		page.set_flag(PageFlag::Present, true);
		page.set_flag(PageFlag::User, process->m_ring == Ring::CPL3);
		page.set_flag(PageFlag::RW, (mapping.flags() & PROT_WRITE));

		current_virt_addr = (void*)((uint64_t)current_virt_addr + 4096);
	}
}

void VMM::notify_free_VMapping(VMapping& mapping) {
#ifdef VMM_LOG_VMAPPING
	kdebugf("[VMM] Freeing VMapping(%x)\n", &mapping);
#endif

	for(auto& page : mapping.pages())
		delete page;
}

PageTable& VMM::ensure_page_table(PageDirectoryEntry& pde) {
	if(pde.get_table())
		return *pde.get_table();

	//  FIXME:  Nasty
	auto* table = reinterpret_cast<PageTable*>(KMalloc::get().kmalloc_alloc(4096, 0x1000));
	if(!table)
		kpanic();
	memset(table, 0x0, 4096);
	pde.set_table(reinterpret_cast<PageTable*>(TO_PHYS(table)));

	return *table;
}
