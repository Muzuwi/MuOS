﻿#include <Arch/i386/PageDirectory.hpp>

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
		kdebugf("[PDE] refusing to set new table: unaligned table address\n");
		return;
	}
	m_data = (m_data & 0xFFF) | ((uint32_t)table);
}

extern uint32_t _ukernel_virtual_offset;
PageTable* PageDirectoryEntry::get_table() const {
	if(m_data == 0)
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
extern uint32_t _ukernel_pages_start;
void PageDirectory::create_table(uint32_t *address) {
	auto& pde = this->get_entry(address);

	if(pde.get_flag(DirectoryFlag::Present) && pde.get_table()) {
		kdebugf("[PD] table already exists at %x\n", pde.get_table());
		return;
	}

	uint32_t *table_start = (uint32_t*)(GET_DIR(address)*1024 - GET_DIR((uint32_t)&_ukernel_virtual_offset)*1024);
//	kdebugf("[PD] start at %x\n", (unsigned)table_start);
	pde.set_table(reinterpret_cast<PageTable*>(table_start));

}
