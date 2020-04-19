#include <Arch/i386/Page.hpp>

/*
 *	Invalidates TLB entries for virt_addr
 */
void invlpg(uintptr_t *virt_addr) {
	asm volatile("mov %%eax, %0\n"
	             "invlpg [%%eax]\t\n"
	             :
	             : ""(virt_addr)
	             : "eax"
	            );
}



/*
 *	Pages
 */
Page::Page() {
	m_data = 0;
}

void Page::set_physical(uintptr_t* phys) {
	m_data &= ~0xFFFFF000;
	m_data |= ((uintptr_t)phys & 0xFFFFF000);
}

bool Page::get_flag(PageFlag flag) const {
	return m_data & (uint32_t)flag;
}

void Page::set_flag(PageFlag flag, bool set) {
	m_data &= ~((uint32_t)flag);
	m_data |= set ? (uint32_t)flag : 0;
}

uint32_t Page::raw() const {
	return m_data;
}

/*
 *	Page Tables
 */
Page& PageTable::get_page(uint32_t *address) {
	return m_pages[GET_PAGE(address)];
}
