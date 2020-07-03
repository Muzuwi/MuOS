#include <Arch/i386/PageDirectory.hpp>
#include <Kernel/Symbols.hpp>
#include <Kernel/Memory/kmalloc.hpp>
#include <Kernel/Memory/VMM.hpp>
#include <Kernel/Memory/PMM.hpp>
#include <Kernel/Memory/QuickMap.hpp>
#include <Kernel/Memory/PageToken.hpp>
#include <string.h>

PageDirectoryEntry::PageDirectoryEntry() {
	m_data = 0;
}

PageDirectoryEntry::PageDirectoryEntry(uint32_t *table_address, uint16_t flags) {
	m_data = ((uint32_t)table_address & 0xFFFFF000) | (flags & 0xFFF);
}

uint16_t PageDirectoryEntry::raw_flags() const {
	return m_data & 0xFFF;
}

bool PageDirectoryEntry::get_flag(DirectoryFlag flag) const {
	return m_data & (uint32_t)flag;
}

void PageDirectoryEntry::set_flag(DirectoryFlag flag, bool set) {
	m_data &= ~((uint32_t)flag);
	m_data |= set ? (uint32_t)flag : 0;
}

void PageDirectoryEntry::set_table(PageTable *table) {
	if(this->get_table()) {
		kdebugf("[PDE] refusing to set new table: table already set (to %x) \n", this->get_table());
		return;
	}

	if((uint32_t)table & 0xFFF) {
		kdebugf("[PDE] refusing to set new table: unaligned table address [%x]\n", (unsigned)table);
		return;
	}
	m_data = (m_data & 0xFFF) | ((uint32_t)table);
}

PageTable* PageDirectoryEntry::get_table() const {
	if((m_data & 0xFFFFF000) == 0)
		return nullptr;
	else
		return reinterpret_cast<PageTable*>((m_data & 0xFFFFF000) + (uint32_t)&_ukernel_virtual_offset);
}

void PageDirectory::dump() {
	for(size_t i = 0; i < 1024; i++) {
		if(m_entries[i].get_flag(DirectoryFlag::Present)) {
			kdebugf("%x-%x is mapped, flags: %x\n", i*0x400000, (i+1)*0x400000, (uint32_t)m_entries[i].raw_flags());
		}
	}
}

/*
 *   Checks if a table is present in this directory for the given virtual address
 */
bool PageDirectory::table_present(uint32_t *address) const {
	return m_entries[GET_DIR(address)].get_flag(DirectoryFlag::Present);
}

/*
 *	Returns a pointer to a page, if one exists, responsible for mapping the
 *  given address
 */
Page* PageDirectory::get_page(uint32_t *address) const {
	if(!table_present(address)) return nullptr;
	else return &(m_entries[GET_DIR(address)]
	                .get_table()
	                ->get_page(address));
}

/*
 *	Returns the PDE containing a given address
 */
PageDirectoryEntry& PageDirectory::get_entry(uint32_t *address) {
	return m_entries[GET_DIR(address)];
}

/*
 *	Loads this Page Directory into cr3
 */
void PageDirectory::load_cr3() {
	 asm volatile(
	    "mov %%eax, %0\n"
	    "mov cr3, %%eax\n"
	    :
	    : ""(&m_entries[0])
	    :
	 );
}

/*
 *	Creates a PageTable that would contain the given virtual address, if one
 *	does not exist already
 */
void PageDirectory::create_table(uint32_t *address) {
	auto& pde = this->get_entry(address);

	if(pde.get_flag(DirectoryFlag::Present) && pde.get_table()) {
		kdebugf("[PD] table already exists at %x\n", pde.get_table());
		return;
	}

	uint32_t pages_start_physical = (uint32_t)&_ukernel_pages_start - (uint32_t)&_ukernel_virtual_offset;
	uint32_t table_start = GET_DIR((uint32_t)address) - GET_DIR((uint32_t)&_ukernel_virtual_offset);
	table_start *= 4096;
	table_start += pages_start_physical;

	pde.set_table(reinterpret_cast<PageTable*>(table_start));

}

/*
 *  Allocates space for a page directory for the current process, and copies over kernel mappings to it
 */
PageDirectory* PageDirectory::create_for_user() {
	//  FIXME: Leaking the PD page here
	auto* page =  PMM::allocate_page_user();
	auto* mem = page->address();

#ifdef LOG_PAGEDIR_CREATION
	kdebugf("[PageDirectory] Allocated PD for user at phys: %x\n", mem);
#endif

	if(mem) {
		auto* kernel_dir = VMM::get_directory();
		QuickMap mapper {mem};
		memset(mapper.address(), 0, 4096);

		auto* kernel_as_arr = reinterpret_cast<uint32_t*>(kernel_dir);
		for(unsigned i = GET_DIR(&_ukernel_virtual_offset); i < 1024; ++i) {
			reinterpret_cast<uint32_t*>(mapper.address())[i] = kernel_as_arr[i];
		}
	} else {
#ifdef LOG_PAGEDIR_CREATION
		kerrorf("[PageDirectory] Page directory creation for user failed!\n");
#endif
	}
	return reinterpret_cast<PageDirectory*>(mem);
}

PageDirectory* PageDirectory::create_for_kernel() {
	//  FIXME: Leaking the PD page here
	auto* page =  PMM::allocate_page_kernel();
	auto* mem = page->address();

#ifdef LOG_PAGEDIR_CREATION
	kdebugf("[PageDirectory] Allocated PD for user at phys: %x\n", mem);
#endif

	if(mem) {
		auto* kernel_dir = VMM::get_directory();
		QuickMap mapper {mem};
		memset(mapper.address(), 0, 4096);

		auto* kernel_as_arr = reinterpret_cast<uint32_t*>(kernel_dir);
		for(unsigned i = GET_DIR(&_ukernel_virtual_offset); i < 1024; ++i) {
			reinterpret_cast<uint32_t*>(mapper.address())[i] = kernel_as_arr[i];
		}
	} else {
#ifdef LOG_PAGEDIR_CREATION
		kerrorf("[PageDirectory] Page directory creation for user failed!\n");
#endif
	}
	return reinterpret_cast<PageDirectory*>(mem);
}
