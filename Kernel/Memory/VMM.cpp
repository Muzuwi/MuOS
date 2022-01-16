#include <Arch/x86_64/CPUID.hpp>
#include <Arch/x86_64/GDT.hpp>
#include <Arch/x86_64/InactiveTaskFrame.hpp>
#include <Kernel/Memory/PMM.hpp>
#include <Kernel/Memory/VMM.hpp>
#include <Kernel/Memory/Units.hpp>
#include <Kernel/Process/Process.hpp>
#include <string.h>

using namespace Units;

/*
 *  Initializes the address space used by the kernel in the kerneld process
 */
void VMM::initialize_kernel_vm() {
	Process& kerneld = Process::_kerneld_ref();

	kdebugf("[VMM] Initializing kerneld address space\n");

	auto res = PMM::instance().allocate();
	if(!res.has_value()) {
		kerrorf("[VMM] Failed allocating page for kerneld PML4! Out of memory?\n");
		kpanic();
	}

	auto pml4 = res.unwrap().base();
	memset(pml4.get_mapped(), 0, 0x1000);
	kerneld.vmm().m_pml4 = res.unwrap().base().as<PML4>();

	kdebugf("[VMM] Mapping kernel executable\n");
	kerneld.vmm()._map_kernel_executable();

	kdebugf("[VMM] Creating physical identity map\n");
	kerneld.vmm()._map_physical_identity();

	kdebugf("[VMM] Preallocating kernel PML4E\n");
	kerneld.vmm()._map_kernel_prealloc_pml4();

	asm volatile(
	"mov %%rax, %0\n"
	"mov cr3, %%rax\n"
	:
	:""(pml4.get())
	:"rax"
	);
}


/*
 *  Maps the given PAllocation starting at virtual address vaddr
 *  Should only be used by lower level kernel allocators, userland should use VMappings
 */
void VMM::_map_pallocation(PAllocation allocation, void* vaddr) {
	auto physical = allocation.base();
	while(physical < allocation.end()) {
		PML4E& pml4e = (*m_pml4)[vaddr];
		auto* pdpte = ensure_pdpt(vaddr, LeakAllocatedPage::Yes);
		auto* pde = ensure_pd(vaddr, LeakAllocatedPage::Yes);
		auto* pte = ensure_pt(vaddr, LeakAllocatedPage::Yes);

		if(!pdpte || !pde || !pte) {
			kerrorf("[VMM] Failed mapping PAllocation for address %x%x! Page structure allocation failed.\n",
			        (uintptr_t)vaddr >> 32u, (uintptr_t)vaddr & 0xffffffffu);
			kpanic();
		}

		pml4e.set(FlagPML4E::Present, true);
		pdpte->set(FlagPDPTE::Present, true);
		pde->set(FlagPDE::Present, true);
		pte->set(FlagPTE::Present, true);
		pte->set(FlagPTE::Global, true);
		pte->set_page(physical);

		physical += 0x1000;
	}
}


/*
 *  Maps the kernel executable in the target process memory.
 *  Only called on kerneld during kernel initialization
 */
void VMM::_map_kernel_executable() {
	auto* const kernel_elf_start = reinterpret_cast<uint8_t*>(&_ukernel_elf_start);
	auto* const kernel_elf_end = reinterpret_cast<uint8_t*>(&_ukernel_elf_end);
	auto* const kernel_text_start = reinterpret_cast<uint8_t*>(&_ukernel_text_start);
	auto* const kernel_text_end = reinterpret_cast<uint8_t*>(&_ukernel_text_end);
	PhysPtr<PML4> pml4 = m_pml4;

	auto kernel_physical = PhysAddr { &_ukernel_physical_start };
	for(auto addr = kernel_elf_start; addr < kernel_elf_end; addr += 0x1000) {
		auto& pml4e = (*pml4)[addr];
		auto* pdpte = ensure_pdpt(addr, LeakAllocatedPage::Yes);
		auto* pde = ensure_pd(addr, LeakAllocatedPage::Yes);
		auto* pte = ensure_pt(addr, LeakAllocatedPage::Yes);
		if(!pdpte || !pde || !pte) {
			kerrorf("[VMM] PDPTE/PDE/PTE allocation failure during ELF mapping. Out of memory?\n");
			kpanic();
		}

		pml4e.set(FlagPML4E::Present, true);
		pml4e.set(FlagPML4E::User, false);
		pdpte->set(FlagPDPTE::Present, true);
		pdpte->set(FlagPDPTE::User, false);
		pde->set(FlagPDE::Present, true);
		pde->set(FlagPDE::User, false);
		pte->set(FlagPTE::Present, true);
		pte->set(FlagPTE::Global, true);
		pte->set(FlagPTE::User, false);
		pte->set_page(kernel_physical);

		if(CPUID::has_NXE()) {
			//  Execute only in text sections
			bool xd = !(addr >= kernel_text_start && addr < kernel_text_end);
			pte->set(FlagPTE::ExecuteDisable, xd);
		}

		kernel_physical += 0x1000;
	}
}


/*
 *  Creates the identity map in the current address space. Only called on kernel
 *  initialization
 */
void VMM::_map_physical_identity() {
	auto identity_start = reinterpret_cast<uint8_t*>(&_ukernel_identity_start);
	auto physical = PhysAddr { nullptr };
	PhysPtr<PML4> pml4 = m_pml4;

	if(CPUID::has_huge_pages()) {
		for(auto addr = identity_start; addr < identity_start + 512 * GiB; addr += 1 * GiB) {
			auto& pml4e = (*pml4)[addr];
			auto* pdpte = ensure_pdpt(addr, LeakAllocatedPage::Yes);
			if(!pdpte) {
				kerrorf("[VMM] PDPTE allocation for physical identity map failed! Out of memory?\n");
				kpanic();
			}

			pml4e.set(FlagPML4E::Present, true);
			pml4e.set(FlagPML4E::User, false);
			pdpte->set(FlagPDPTE::Present, true);
			pdpte->set(FlagPDPTE::User, false);
			pdpte->set(FlagPDPTE::HugePage, true);
			pdpte->set_directory(physical.as<PD>());

			physical += 1 * Units::GiB;
		}
	} else {
		for(auto addr = identity_start; addr < identity_start + 512 * GiB; addr += 2 * MiB) {
			auto& pml4e = (*pml4)[addr];
			auto* pdpte = ensure_pdpt(addr, LeakAllocatedPage::Yes);
			auto* pde = ensure_pd(addr, LeakAllocatedPage::Yes);
			if(!pdpte || !pde) {
				kerrorf("[VMM] PDPTE/PDE allocation for physical identity map failed! Out of memory?\n");
				kpanic();
			}

			pml4e.set(FlagPML4E::Present, true);
			pml4e.set(FlagPML4E::User, false);
			pdpte->set(FlagPDPTE::Present, true);
			pdpte->set(FlagPDPTE::User, false);
			pde->set(FlagPDE::Present, true);
			pde->set(FlagPDE::User, false);
			pde->set(FlagPDE::LargePage, true);
			pde->set_table(physical.as<PT>());

			physical += 2 * MiB;
		}
	}
}


/*
 *  Preallocate all PML4 PDPT nodes for the kernel shared address space
 *  That way, any changes to the shared kernel address space will persist throughout all processes,
 *  as all process PML4's are clones of the root PML4
 */
void VMM::_map_kernel_prealloc_pml4() {
	PhysPtr<PML4> pml4 = m_pml4;
	for(unsigned i = index_pml4e(&_ukernel_shared_start); i <= index_pml4e(&_ukernel_shared_end); ++i) {
		auto addr = (void*)((uint64_t)i << 39ul);
		(*pml4)[addr].set(FlagPML4E::Present, true);

		ensure_pdpt(addr, LeakAllocatedPage::Yes);
	}
}


PDPTE* VMM::ensure_pdpt(void* addr, LeakAllocatedPage leak) {
	PML4E& pml4e = (*m_pml4)[addr];

	if(!pml4e.directory()) {
		PhysAddr phys;
		if(leak == LeakAllocatedPage::Yes) {
			KOptional<PAllocation> alloc = PMM::instance().allocate(0);
			if(!alloc.has_value()) {
				kerrorf("[VMM] Failed to allocate page for PT!\n");
				return nullptr;
			}
			phys = alloc.unwrap().base();
		} else {
			KOptional<PhysAddr> alloc = _allocate_kernel_page(0);
			if(!alloc.has_value()) {
				kerrorf("[VMM] Failed to allocate page for PT!\n");
				return nullptr;
			}
			phys = alloc.unwrap();
		}

		memset(phys.get_mapped(), 0x0, 0x1000);
		pml4e.set_directory(phys.as<PDPT>());
	}

	return &(*pml4e.directory())[addr];
}

PDE* VMM::ensure_pd(void* addr, LeakAllocatedPage leak) {
	auto* pdpte = ensure_pdpt(addr, leak);
	if(!pdpte) { return nullptr; }

	if(!pdpte->directory()) {
		PhysAddr phys;
		if(leak == LeakAllocatedPage::Yes) {
			KOptional<PAllocation> alloc = PMM::instance().allocate(0);
			if(!alloc.has_value()) {
				kerrorf("[VMM] Failed to allocate page for PD!\n");
				return nullptr;
			}
			phys = alloc.unwrap().base();
		} else {
			KOptional<PhysAddr> alloc = _allocate_kernel_page(0);
			if(!alloc.has_value()) {
				kerrorf("[VMM] Failed to allocate page for PD!\n");
				return nullptr;
			}
			phys = alloc.unwrap();
		}

		memset(phys.get_mapped(), 0x0, 0x1000);
		pdpte->set_directory(phys.as<PD>());
	}

	return &(*pdpte->directory())[addr];
}

PTE* VMM::ensure_pt(void* addr, LeakAllocatedPage leak) {
	auto* pde = ensure_pd(addr, leak);
	if(!pde) { return nullptr; }

	if(!pde->table()) {
		PhysAddr phys;
		if(leak == LeakAllocatedPage::Yes) {
			KOptional<PAllocation> alloc = PMM::instance().allocate(0);
			if(!alloc.has_value()) {
				kerrorf("[VMM] Failed to allocate page for PT!\n");
				return nullptr;
			}
			phys = alloc.unwrap().base();
		} else {
			KOptional<PhysAddr> alloc = _allocate_kernel_page(0);
			if(!alloc.has_value()) {
				kerrorf("[VMM] Failed to allocate page for PT!\n");
				return nullptr;
			}
			phys = alloc.unwrap();
		}

		memset(phys.get_mapped(), 0x0, 0x1000);
		pde->set_table(phys.as<PT>());
	}

	return &(*pde->table())[addr];
}

KOptional<PhysPtr<PML4>> VMM::clone_pml4(PhysPtr<PML4> source) {
	auto page = _allocate_kernel_page(0);
	if(!page.has_value()) {
		return {};
	}

	auto pml4 = page.unwrap().as<PML4>();
	//  Clone flags, dirs should be overwritten
	memcpy(pml4.get_mapped(), source.get_mapped(), 0x1000);

	for(unsigned i = 0; i < 512; ++i) {
		//  Kernel shared memory should be copied as-is
		if(i >= index_pml4e(&_ukernel_shared_start) && i <= index_pml4e(&_ukernel_shared_end)) {
			pml4->m_entries[i] = source->m_entries[i];
			continue;
		}

		//  Clone only present entries
		if(pml4->m_entries[i].get(FlagPML4E::Present)) {
			auto pdpt = clone_pdpt(pml4->m_entries[i].directory());
			if(!pdpt.has_value()) {
				return {};
			}

			pml4->m_entries[i].set_directory(pdpt.unwrap());
		}
	}

	return pml4;
}

KOptional<PhysPtr<PDPT>> VMM::clone_pdpt(PhysPtr<PDPT> source) {
	auto page = _allocate_kernel_page(0);
	if(!page.has_value()) {
		return {};
	}

	auto pdpt = page.unwrap().as<PDPT>();
	//  Clone flags, dirs should be overwritten
	memcpy(pdpt.get_mapped(), source.get_mapped(), 0x1000);

	for(unsigned i = 0; i < 512; ++i) {
		//  Skip entries marked as huge page - these do not contain any paging structures but physical addresses
		if(pdpt->m_entries[i].get(FlagPDPTE::HugePage)) {
			continue;
		}

		//  Clone only present entries
		if(pdpt->m_entries[i].get(FlagPDPTE::Present)) {
			auto pd = clone_pd(pdpt->m_entries[i].directory());
			if(!pd.has_value()) {
				return {};
			}

			pdpt->m_entries[i].set_directory(pd.unwrap());
		}
	}

	return pdpt;

}

KOptional<PhysPtr<PD>> VMM::clone_pd(PhysPtr<PD> source) {
	auto page = _allocate_kernel_page(0);
	if(!page.has_value()) {
		return {};
	}

	auto pd = page.unwrap().as<PD>();
	//  Clone flags, dirs should be overwritten
	memcpy(pd.get_mapped(), source.get_mapped(), 0x1000);

	for(unsigned i = 0; i < 512; ++i) {
		//  Skip entries marked as large page - these do not contain any paging structures but physical addresses
		if(pd->m_entries[i].get(FlagPDE::LargePage)) {
			continue;
		}

		//  Clone only present entries
		if(pd->m_entries[i].get(FlagPDE::Present)) {
			auto pt = clone_pt(pd->m_entries[i].table());
			if(!pt.has_value()) {
				return {};
			}

			pd->m_entries[i].set_table(pt.unwrap());
		}
	}

	return pd;
}

KOptional<PhysPtr<PT>> VMM::clone_pt(PhysPtr<PT> source) {
	auto page = _allocate_kernel_page(0);
	if(!page.has_value()) {
		return {};
	}

	auto pt = page.unwrap().as<PT>();
	//  Clone flags, dirs should be overwritten
	memcpy(pt.get_mapped(), source.get_mapped(), 0x1000);

	return { pt };
}


/*
 *  Maps a given VMapping in the target process
 */
bool VMM::map(VMapping const& mapping) {
	auto virtual_addr = (uint8_t*)mapping.addr();
	for(auto& page : mapping.pages()) {
		auto phys = page.base();
		for(unsigned i = 0; i < (1u << page.order()); ++i) {
			addrmap(virtual_addr, phys, static_cast<VMappingFlags>(mapping.flags()));
			virtual_addr += 0x1000;
			phys += 0x1000;
		}
	}
	return true;
}


/*
 *  Unmaps the given VMapping from the target process
 */
bool VMM::unmap(VMapping const& mapping) {
	auto virtual_addr = (uint8_t*)mapping.addr();
	for(auto& page : mapping.pages()) {
		auto phys = page.base();
		for(unsigned i = 0; i < (1u << page.order()); ++i) {
			addrunmap(virtual_addr);
			virtual_addr += 0x1000;
			phys += 0x1000;
		}
	}
	return true;
}


/*
 *  Map a virtual address to a physical address
 */
bool VMM::addrmap(void* vaddr, PhysAddr paddr, VMappingFlags flags) {
	PhysPtr<PML4> pml4 = m_pml4;

	auto& pml4e = (*pml4)[vaddr];
	auto* pdpte = ensure_pdpt(vaddr, LeakAllocatedPage::No);
	auto* pde = ensure_pd(vaddr, LeakAllocatedPage::No);
	auto* pte = ensure_pt(vaddr, LeakAllocatedPage::No);
	if(!pdpte || !pde || !pte) {
		kerrorf("[VMM] VMapping map failed - PDPTE/PDE/PTE allocation failed.\n");
		return false;
	}

	//  For top-level structures (above PTEs), set the most permissive flags
	//  (also set proper user/supervisor flags based on virtual address, not the mapping flags)
	const bool is_user_mem = index_pml4e(vaddr) < index_pml4e(&_ukernel_virtual_start);

	pml4e.set(FlagPML4E::Present, true);
	pml4e.set(FlagPML4E::User, is_user_mem);
	pml4e.set(FlagPML4E::RW, true);

	pdpte->set(FlagPDPTE::Present, true);
	pdpte->set(FlagPDPTE::User, is_user_mem);
	pdpte->set(FlagPDPTE::RW, true);

	pde->set(FlagPDE::Present, true);
	pde->set(FlagPDE::User, is_user_mem);
	pde->set(FlagPDE::RW, true);

	pte->set(FlagPTE::Present, flags & VM_READ);
	pte->set(FlagPTE::User, !(flags & VM_KERNEL));
	pte->set(FlagPTE::RW, flags & VM_WRITE);
	pte->set(FlagPTE::ExecuteDisable, !(flags & VM_EXEC));
	pte->set_page(paddr);

	return false;
}


/*
 *  Unmap the page with the specified virtual address
 */
bool VMM::addrunmap(void* vaddr) {
	PhysPtr<PML4> pml4 = m_pml4;
	auto& pml4e = (*pml4)[vaddr];
	if(!pml4e.get(FlagPML4E::Present)) {
		kerrorf("[VMM] VMapping corruption or double free detected\n");
		kpanic();
	}
	auto& pdpte = (*pml4e.directory())[vaddr];
	if(!pdpte.get(FlagPDPTE::Present)) {
		kerrorf("[VMM] VMapping corruption or double free detected\n");
		kpanic();
	}
	auto& pde = (*pdpte.directory())[vaddr];
	if(!pde.get(FlagPDE::Present)) {
		kerrorf("[VMM] VMapping corruption or double free detected\n");
		kpanic();
	}
	auto& pte = (*pde.table())[vaddr];
	pte.set(FlagPTE::Present, false);

	return true;
}


/*
 *  Allocates a physical page for use in the kernel, on behalf of the current process.
 *  After the process is removed, these pages WILL BE FREED.
 *  Can't use VMappings for these, as most often they won't have an underlying VM mapping
 */
KOptional<PhysAddr> VMM::_allocate_kernel_page(size_t order) {
	auto alloc = PMM::instance().allocate(order);
	if(!alloc.has_value()) {
		return {};
	}

	m_kernel_pages.push_back(alloc.unwrap());
	return { alloc.unwrap().base() };
}


/*
 *  Looks for a VMapping for the given virtual address
 */
KOptional<SharedPtr<VMapping>> VMM::find_vmapping(void* vaddr) const {
	for(auto& vmapping : m_mappings) {
		if(!vmapping) { continue; }
		if(!vmapping->contains(vaddr)) { continue; }

		return { vmapping };
	}

	return {};
}


/*
 *  Validates whether the given VMapping does not overlap with any other mappings,
 *  and saves it into the list
 */
bool VMM::insert_vmapping(SharedPtr<VMapping>&& mapping) {
	if(!mapping) { return false; }

	auto find_vmapping_iterator = [this](void* address) {
		auto it = m_mappings.begin();
		while(it != m_mappings.end() && (*it)->addr() < address)
			++it;

		return it;
	};

	auto it = find_vmapping_iterator(mapping->addr());
	if(it == m_mappings.end()) {
		m_mappings.push_back(mapping);
		return map(*mapping);
	}

	if(mapping->overlaps(**it)) {
		return false;
	}

	m_mappings.insert(it, mapping);
	return map(*mapping);
}


void* VMM::allocate_user_stack(uint64 stack_size) {
	//  FIXME: Actually randomize
	auto random = [](size_t, size_t) -> size_t {
		return 4;
	};

	uint64_t stack_top;
	if(m_process.flags().randomize_vm) {
		stack_top = (uintptr_t)&_userspace_stack_start
		            + VMM::user_stack_size() * random(0, 0x40000);
	} else {
		stack_top = (uintptr_t)&_userspace_stack_start;
	}

	auto stack_mapping = VMapping::create((void*)stack_top, VMM::user_stack_size(), VM_READ | VM_WRITE, MAP_PRIVATE);
	kassert(insert_vmapping(gen::move(stack_mapping)));

	return (void*)(stack_top + VMM::user_stack_size());
}


VMapping* VMM::allocate_kernel_stack(uint64 stack_size) {
	void* kstack_top = m_next_kernel_stack_at;
	void* kstack_bottom = (void*)((uintptr_t)m_next_kernel_stack_at + stack_size);

	//  FIXME/LIMITS
	if(kstack_bottom > &_ukernel_virt_kstack_end) {
		return {};
	}
	//  FIXME: Handle randomize_vm flag

	m_next_kernel_stack_at = (void*)((uintptr_t)m_next_kernel_stack_at + stack_size + 0x1000);

	auto mapping = VMapping::create(
			(void*)kstack_top,
			stack_size,
			VM_READ | VM_WRITE | VM_KERNEL,
			MAP_PRIVATE
	                               );
	auto mapping_ptr = mapping.get();
	kassert(insert_vmapping(gen::move(mapping)));

	return mapping_ptr;
}


void* VMM::allocate_user_heap(size_t region_size) {
	size_t size_rounded = region_size & ~(0x1000 - 1);
	if(size_rounded == 0) {
		return (void*)(-1);
	}

	auto addr = m_next_anon_vm_at;
	auto vmapping = VMapping::create(addr, size_rounded, VM_READ | VM_WRITE, MAP_PRIVATE);

	if(!insert_vmapping(gen::move(vmapping))) {
		return (void*)(-1);
	}

	m_next_anon_vm_at = (void*)((uint64)m_next_anon_vm_at + size_rounded);

	return addr;
}
