#include <Arch/i386/PageDirectory.hpp>

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

extern uint32_t _ukernel_virtual_offset;
PageTable* PageDirectoryEntry::get_table() const {
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
