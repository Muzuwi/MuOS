#include <Kernel/Process/Process.hpp>
#include <Kernel/Memory/kmalloc.hpp>
#include <Kernel/Memory/QuickMap.hpp>
#include <Arch/i386/IRQDisabler.hpp>

PageTable* s_table_for_quick_map;
gen::BitMap s_free_pages {1024};

QuickMap::QuickMap(void* addr) {
	IRQDisabler disabler;

	if((uint64_t)addr % 4096 != 0) {
		kerrorf("QuickMapper called with unaligned address! Correcting it\n");
		addr = (void*)((uint64_t)addr & ~0xfff);
	}

	if(!s_table_for_quick_map) {
		kdebugf("QuickMapper not initialized, initializing.\n");
		s_table_for_quick_map = reinterpret_cast<PageTable*>(KMalloc::get().kmalloc_alloc(0x1000, 0x1000));

		uint32_t cr3 = 0;
		asm volatile(
				"mov %0, %%cr3\n"
				:"=a"(cr3));

		auto* currentDir = reinterpret_cast<PageDirectory*>(TO_VIRT(cr3));

		auto& entry = currentDir->get_entry((uint32_t*)(QUICKMAP_VIRT_START));
		entry.set_table(reinterpret_cast<PageTable*>(TO_PHYS(s_table_for_quick_map)));
		entry.set_flag(DirectoryFlag::Present, true);
		entry.set_flag(DirectoryFlag::RW, true);
		entry.set_flag(DirectoryFlag::User, false);
	}

	m_index = s_free_pages.find_seq_clear(1);
	assert(m_index != s_free_pages.count());

	auto& page = s_table_for_quick_map->get_page(reinterpret_cast<uint32_t*>(address()));
	page.set_flag(PageFlag::Present, true);
	page.set_flag(PageFlag::RW, true);
	page.set_flag(PageFlag::User, false);
	page.set_flag(PageFlag::Global, true);
	page.set_physical((uintptr_t *)addr);
	invlpg((uintptr_t*)address());

	kdebugf("QuickMap(%x): mapped %x to %x\n", this, addr, address());
}

void* QuickMap::address() {
	return reinterpret_cast<void*>(QUICKMAP_VIRT_START + m_index * 0x1000);
}

QuickMap::~QuickMap() {
	kdebugf("QuickMap(%x): freed %x\n", this, address());
	s_free_pages.set(m_index, false);
	auto& page = s_table_for_quick_map->get_page(reinterpret_cast<uint32_t*>(m_index * 0x1000));
	page.set_flag(PageFlag::Present, false);
	page.set_physical((uintptr_t *)nullptr);
}
