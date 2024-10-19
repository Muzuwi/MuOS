#include "VM.hpp"
#include <Arch/VM.hpp>
#include <Arch/x86_64/CPUID.hpp>
#include <Arch/x86_64/LinkscriptSyms.hpp>
#include <Core/Assert/Panic.hpp>
#include <Core/Error/Error.hpp>
#include <Core/Mem/GFP.hpp>
#include <string.h>
#include <SystemTypes.hpp>

//  Identity map the given paging handle
static inline arch::PagingTable* idmap_handle(arch::PagingHandle handle) {
	return reinterpret_cast<arch::PagingTable*>(idmap(handle));
}

//	Identity map the table pointed to by the given paging entry
static inline arch::PagingTable* idmap_entry_to_table(arch::PagingEntry* entry) {
	if(!entry) {
		return nullptr;
	}
	return reinterpret_cast<arch::PagingTable*>(idmap(entry->getaddr()));
}

core::Result<arch::PagingHandle> arch::addralloc() {
	auto maybe_page = core::mem::allocate_pages(0, {});
	if(maybe_page.has_error()) {
		return core::Result<arch::PagingHandle> { core::Error::NoMem };
	}
	auto allocation = maybe_page.destructively_move_data();
	memset(idmap(allocation.base), 0x0, allocation.size());
	return core::Result<arch::PagingHandle> { static_cast<PagingHandle>(allocation.base) };
}

core::Error arch::addrfree(PagingHandle) {
	//  TODO: Walk the page tables and free all intermediate pages we've allocated
	core::panic("arch::addrfree unimplemented on x86_64!");
}

//  Clone paging tables
//  `Level` template parameter determines the type of the table cloned:
//  	0: PT
//  	1: PD
//  	2: PDPT
//  	3: PML4
template<size_t Level = 3>
core::Result<arch::PagingTable*> clone_table(arch::PagingTable* table) {
	using namespace arch;
	//  Large/huge page flag is located at the same bit
	static constexpr uint64 LARGE_PAGE_FLAG = 1U << 7U;

	auto maybe_page = core::mem::allocate_pages(0, {});
	if(maybe_page.has_error()) {
		return core::Result<arch::PagingTable*> { core::Error::NoMem };
	}
	auto allocation = maybe_page.destructively_move_data();
	auto* new_table = idmap_handle(allocation.base);

	//  Copy over the original table contents
	memcpy(new_table, table, 0x1000);

	//  Now post-process the new table. All entries are exactly the same
	//  as in the old table, but we need to clone the next-level tables
	//  as well to complete the deep clone.
	for(size_t i = 0; i < 512; ++i) {
		if constexpr(Level == 3) {
			//  For shared VM memory, copy the entry directly and do NOT clone
			//  the table. This is a performance/simplicity optimization, as the
			//  alternative would involve modifying every single processes' address
			//  space when any change to the kernel VM is made.
			if(i >= index_pml4e(KERNEL_VM_SHARED_START) && i <= index_pml4e(KERNEL_VM_SHARED_END)) {
				continue;
			}
		}
		if constexpr(Level == 2 || Level == 1) {
			//  If this table can contain large/huge pags, and the entry has the flag set,
			//  do not process it, as it won't contain a table address.
			if(new_table->data[i] & LARGE_PAGE_FLAG) {
				continue;
			}
		}
		auto* entry = reinterpret_cast<arch::PagingEntry*>(new_table->data + i);
		if(entry->get(arch::EntryFlags::Present)) {
			auto maybe_subtable = clone_table<Level - 1>(idmap_handle(entry->getaddr()));
			if(maybe_subtable.has_error()) {
				return core::Result<arch::PagingTable*> { core::Error::NoMem };
			}
			arch::PagingTable* subtable = maybe_subtable.destructively_move_data();
			entry->setaddr(subtable);
		}
	}

	return core::Result<arch::PagingTable*> { new_table };
}

//  Clone paging tables
//  Specialization for Level=0 to terminate recursive template instantiation
//  Here, table points to a PT.
template<>
core::Result<arch::PagingTable*> clone_table<0>(arch::PagingTable* table) {
	auto maybe_page = core::mem::allocate_pages(0, {});
	if(maybe_page.has_error()) {
		return core::Result<arch::PagingTable*> { core::Error::NoMem };
	}
	auto allocation = maybe_page.destructively_move_data();
	auto* new_table = idmap_handle(allocation.base);

	//  Copy over the original table contents
	memcpy(new_table, table, 0x1000);
	return core::Result<arch::PagingTable*> { new_table };
}

core::Result<arch::PagingHandle> arch::addrclone(arch::PagingHandle handle) {
	if(!handle) {
		return core::Result<arch::PagingHandle> { core::Error::InvalidArgument };
	}

	auto maybe_pml4 = clone_table<>(idmap_handle(handle));
	if(maybe_pml4.has_error()) {
		return core::Result<arch::PagingHandle> { core::Error::NoMem };
	}
	auto* pml4 = maybe_pml4.destructively_move_data();

	//  Cloning returns an easily accessible pointer in identity map, convert
	//  it back to a physical address.
	return core::Result<arch::PagingHandle> { reinterpret_cast<arch::PagingHandle*>(idunmap(pml4)) };
}

template<auto IndexerMemberFunction>
static arch::PagingTable* ensure_table(arch::PagingTable* table, void* vptr) {
	using namespace arch;

	arch::PagingEntry* table_entry = (table->*IndexerMemberFunction)(vptr);
	if(!table_entry->get(arch::EntryFlags::Present)) {
		auto maybe_page = core::mem::allocate_pages(0, {});
		if(maybe_page.has_error()) {
			return nullptr;
		}
		auto page = maybe_page.destructively_move_data();
		memset(idmap(page.base), 0x0, 0x1000);

		//  For top-level structures (above PTEs), set the most permissive flags. Also set
		//  user/supervisor flags on intermediates based on virtual address, not the mapping
		//  flags. This is so all memory above the kernel virtual start will ALWAYS be marked
		//  as supervisor-only, thus inaccessible to userland, even if addrmap is somehow called
		//  with PageFlags::User.
		//  Basically: kernel-only page in user virtual - OK, user-accessible page in kernel virtual - BAD
		const bool is_user_mem = index_pml4e(vptr) < index_pml4e(KERNEL_VM_START);
		table_entry->reset(EntryFlags::Present | EntryFlags::RW);
		if(is_user_mem) {
			table_entry->set(EntryFlags::User, true);
		}
		table_entry->setaddr(page.base);
	}
	return idmap_entry_to_table(table_entry);
}

static constexpr arch::EntryFlags entry_flags_from_request(arch::PageFlags flags) {
	using namespace arch;
	auto entry_flags = EntryFlags::Present;
	if(flags & PageFlags::User) {
		entry_flags = entry_flags | EntryFlags::User;
	}
	if(flags & PageFlags::Write) {
		entry_flags = entry_flags | EntryFlags::RW;
	}
	if(!(flags & PageFlags::Execute)) {
		entry_flags = entry_flags | EntryFlags::ExecuteDisable;
	}
	return entry_flags;
}

static core::Error addrmap_4k(arch::PagingHandle handle, void* pptr, void* vptr, arch::PageFlags flags) {
	using namespace arch;

	auto* pml4 = idmap_handle(handle);
	auto* pdpt = ensure_table<&PagingTable::get_pml4e>(pml4, vptr);
	if(!pdpt) {
		return core::Error::NoMem;
	}
	auto* pd = ensure_table<&PagingTable::get_pdpte>(pdpt, vptr);
	if(!pd) {
		return core::Error::NoMem;
	}
	auto* pt = ensure_table<&PagingTable::get_pde>(pd, vptr);
	if(!pt) {
		return core::Error::NoMem;
	}
	auto* pte = pt->get_pte(vptr);
	pte->reset(entry_flags_from_request(flags));
	pte->setaddr(pptr);

	return core::Error::Ok;
}

static core::Error addrmap_2m(arch::PagingHandle handle, void* pptr, void* vptr, arch::PageFlags flags) {
	using namespace arch;

	if(!is_large_page_aligned(vptr) || !is_large_page_aligned(pptr)) {
		return core::Error::InvalidArgument;
	}

	auto* pml4 = idmap_handle(handle);
	auto* pdpt = ensure_table<&PagingTable::get_pml4e>(pml4, vptr);
	if(!pdpt) {
		return core::Error::NoMem;
	}
	auto* pd = ensure_table<&PagingTable::get_pdpte>(pdpt, vptr);
	if(!pd) {
		return core::Error::NoMem;
	}
	auto* pde = pd->get_pde(vptr);
	pde->reset(entry_flags_from_request(flags));
	pde->set(FlagPDE::LargePage, true);
	pde->setaddr(pptr);

	return core::Error::Ok;
}

static core::Error addrmap_1g(arch::PagingHandle handle, void* pptr, void* vptr, arch::PageFlags flags) {
	using namespace arch;

	if(!CPUID::has_huge_pages()) {
		return core::Error::Unsupported;
	}
	if(!is_huge_page_aligned(vptr) || !is_huge_page_aligned(pptr)) {
		return core::Error::InvalidArgument;
	}

	auto* pml4 = idmap_handle(handle);
	auto* pdpt = ensure_table<&PagingTable::get_pml4e>(pml4, vptr);
	if(!pdpt) {
		return core::Error::NoMem;
	}
	auto* pdpte = pdpt->get_pdpte(vptr);
	pdpte->reset(entry_flags_from_request(flags));
	pdpte->set(FlagPDPTE::HugePage, true);
	pdpte->setaddr(pptr);

	return core::Error::Ok;
}

core::Error arch::addrmap(PagingHandle handle, void* pptr, void* vptr, PageFlags flags) {
	if(!handle) {
		return core::Error::InvalidArgument;
	}
	if(!(flags & PageFlags::Execute) && !CPUID::has_NXE()) {
		return core::Error::Unsupported;
	}
	if(flags & PageFlags::Large) [[unlikely]] {
		return addrmap_2m(handle, pptr, vptr, flags);
	}
	if(flags & PageFlags::Huge) [[unlikely]] {
		return addrmap_1g(handle, pptr, vptr, flags);
	}
	return addrmap_4k(handle, pptr, vptr, flags);
}

core::Error arch::addrunmap(arch::PagingHandle handle, void* vptr) {
	if(!handle || !is_page_aligned(vptr)) {
		return core::Error::InvalidArgument;
	}

	auto* pml4 = idmap_handle(handle);
	auto* pml4e = pml4->get_pml4e(vptr);
	if(!pml4e->get(EntryFlags::Present)) {
		return core::Error::EntityMissing;
	}

	auto* pdpt = idmap_entry_to_table(pml4e);
	auto* pdpte = pdpt->get_pdpte(vptr);
	if(!pdpte->get(EntryFlags::Present)) {
		return core::Error::EntityMissing;
	}
	if(pdpte->get(FlagPDPTE::HugePage)) {
		if(!is_huge_page_aligned(vptr)) {
			return core::Error::InvalidArgument;
		}
		pdpte->set(EntryFlags::Present, false);
		pdpte->setaddr(nullptr);
		return core::Error::Ok;
	}

	auto* pd = idmap_entry_to_table(pdpte);
	auto* pde = pd->get_pde(vptr);
	if(!pde->get(EntryFlags::Present)) {
		return core::Error::EntityMissing;
	}
	if(pde->get(FlagPDE::LargePage)) {
		if(!is_large_page_aligned(vptr)) {
			return core::Error::InvalidArgument;
		}
		pde->set(EntryFlags::Present, false);
		pde->setaddr(nullptr);
		return core::Error::Ok;
	}

	auto* pt = idmap_entry_to_table(pde);
	auto* pte = pt->get_pte(vptr);
	if(!pte->get(EntryFlags::Present)) {
		return core::Error::EntityMissing;
	}

	pte->set(EntryFlags::Present, false);
	pte->setaddr(nullptr);

	return core::Error::Ok;
}
