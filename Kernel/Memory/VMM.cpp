#include <string.h>
#include <Arch/i386/Paging.hpp>
#include <Kernel/Debug/kdebugf.hpp>
#include <Kernel/Debug/kpanic.hpp>
#include <Kernel/Memory/PMM.hpp>
#include <Kernel/Memory/Units.hpp>
#include <Kernel/Memory/VMM.hpp>
#include <Kernel/Symbols.hpp>

using Units::MiB;

static PhysPtr<PML4> s_kernel_pml4 {nullptr};

void VMM::init() {
	kdebugf("[VMM] Init kernel PML4\n");

	auto res = PMM::allocate();
	if(!res.has_value()) {
		kerrorf("[VMM] Failed to allocate page for PML4!\n");
		kpanic();
	}
	auto pml4 = res.unwrap().base();
	memset(pml4.get_mapped(), 0, 0x1000);
	s_kernel_pml4 = pml4.as<PML4>();

	kdebugf("[VMM] Mapping kernel executable\n");
	map_kernel_executable();
	kdebugf("[VMM] Creating physical identity map\n");
	map_physical_identity();

	kdebugf("[VMM] Reloading cr3\n");
	asm volatile(
	"mov %%rax, %0\n"
	"mov cr3, %%rax\n"
	:
	:""(s_kernel_pml4.get())
	:"rax"
	);
	kdebugf("[VMM] Init complete\n");
}

#define SPLITPTR(a) (uintptr_t)a>>32u, (uintptr_t)a&0xffffffffu

void VMM::map_kernel_executable() {
	auto kernel_elf_start = reinterpret_cast<uint8_t*>(&_ukernel_elf_start);
	auto kernel_elf_end = reinterpret_cast<uint8_t*>(&_ukernel_elf_end);

	auto kernel_physical = PhysAddr{&_ukernel_physical_start};
	for(auto addr = kernel_elf_start; addr < kernel_elf_end; addr += 0x1000) {
		auto& pml4e = (*s_kernel_pml4)[addr];
		auto& pdpte = ensure_pdpte(addr);
		auto& pde = ensure_pde(addr);
		auto& pte = ensure_pte(addr);

		pml4e.set(FlagPML4E::Present, true);
		pml4e.set(FlagPML4E::User, false);
		pdpte.set(FlagPDPTE::Present, true);
		pdpte.set(FlagPDPTE::User, false);
		pde.set(FlagPDE::Present, true);
		pde.set(FlagPDE::User, false);
		pte.set(FlagPTE::Present, true);
		pte.set(FlagPTE::Global, true);
		pte.set(FlagPTE::User, false);
		pte.set_page(kernel_physical);
		kernel_physical += 4096;
	}
}

void VMM::map_physical_identity() {
	auto identity_start = reinterpret_cast<uint8_t*>(&_ukernel_identity_start);
	auto physical = PhysAddr{nullptr};

	//  FIXME: Assuming support for 2MiB pages
	//  FIXME: Use hugepages when available
	for(auto addr = identity_start; addr < identity_start + 0x8000000000; addr += 2 * MiB) {
		auto& pml4e = (*s_kernel_pml4)[addr];
		auto& pdpte = ensure_pdpte(addr);
		auto& pde = ensure_pde(addr);

		pml4e.set(FlagPML4E::Present, true);
		pml4e.set(FlagPML4E::User, false);
		pdpte.set(FlagPDPTE::Present, true);
		pdpte.set(FlagPDPTE::User, false);
		pde.set(FlagPDE::Present, true);
		pde.set(FlagPDE::User, false);
		pde.set(FlagPDE::LargePage, true);
		pde.set_table(physical.as<PT>());

		physical += 2*MiB;
	}
}

PDPTE& VMM::ensure_pdpte(void* addr) {
	auto& pml4e = (*s_kernel_pml4)[addr];
	if(!pml4e.directory()) {
		auto alloc = PMM::allocate();
		if(!alloc.has_value()) {
			kerrorf("[VMM] Failed to allocate page for PDPT!\n");
			kpanic();
		}
		memset(alloc.unwrap().base().get_mapped(), 0x0, 0x1000);
		pml4e.set_directory(alloc.unwrap().base().as<PDPT>());
	}

	return (*pml4e.directory())[addr];
}

PDE& VMM::ensure_pde(void* addr) {
	auto& pdpte = ensure_pdpte(addr);
	if(!pdpte.directory()) {
		auto alloc = PMM::allocate();
		if(!alloc.has_value()) {
			kerrorf("[VMM] Failed to allocate page for PD!\n");
			kpanic();
		}
		memset(alloc.unwrap().base().get_mapped(), 0x0, 0x1000);
		pdpte.set_directory(alloc.unwrap().base().as<PD>());
	}

	return (*pdpte.directory())[addr];
}

PTE& VMM::ensure_pte(void* addr) {
	auto& pde = ensure_pde(addr);
	if(!pde.table()) {
		auto alloc = PMM::allocate();
		if(!alloc.has_value()) {
			kerrorf("[VMM] Failed to allocate page for PT!\n");
			kpanic();
		}
		memset(alloc.unwrap().base().get_mapped(), 0x0, 0x1000);
		pde.set_table(alloc.unwrap().base().as<PT>());
	}

	return (*pde.table())[addr];
}


//
//uint32_t s_kernel_directory_table[1024] __attribute__((aligned(4096)));
//PageDirectory* VMM::s_kernel_directory = reinterpret_cast<PageDirectory*>(&s_kernel_directory_table[0]);
//
///*
// *	Initializes the memory manager of the kernel
// *  TODO: Set permissions on the kernel executable sections
// */
//void VMM::init() {
//	kdebugf("[VMM] Removing identity mappings\n");
//
//	//  FIXME:  Add a method to PageDirectory that allows unmapping entire tables
//	//  and actually flushes them afterwards
//	s_kernel_directory->get_entry((uint32_t*)0x0).set_flag(DirectoryFlag::Present, false);
//
//	kdebugf("[VMM] Locking write on read-only regions\n");
//	uintptr_t cur = (uintptr_t)&_ukernel_RO_begin;
//	while(cur < (uintptr_t)&_ukernel_RO_end) {
//		auto* page = s_kernel_directory->get_page((uint32_t*)cur);
//		if(page) {
//			page->set_flag(PageFlag::RW, false);
//			invlpg((uintptr_t*)cur);
//		}
//
//		cur += 4096;
//	}
//
//}
//
///*
// *	Returns the kernel page directory
// */
//PageDirectory* VMM::get_directory() {
//	return s_kernel_directory;
//}
//
//VMM& VMM::get(){
//	static VMM virtual_memory_manager;
//	return virtual_memory_manager;
//}
//
///*
// *  Allocates stack space with given size for the current process
// */
//void* VMM::allocate_user_stack(size_t stack_size) {
//	//  Process stacks are allocated right before the kernel virtual address space
//	auto* stack_top = (void*)((uint32_t)&_ukernel_virtual_offset-stack_size);
//	auto& mapping = VMapping::create_for_user(stack_top, stack_size, PROT_READ | PROT_WRITE, MAP_SHARED);
//
//	return (void*)((uint32_t)&_ukernel_virtual_offset);
//}
//
//void VMM::notify_create_VMapping(gen::SharedPtr<VMapping> mapping, MappingPrivilege access) {
//	ASSERT_IRQ_DISABLED();
//#ifdef VMM_LOG_VMAPPING
//	kdebugf("[VMM] Created VMapping(%x)\n", &mapping);
//#endif
//	assert((bool)mapping);
//
//	auto* process = Process::m_current;
//	QuickMap mapper {process->m_directory};
//	auto* dir = reinterpret_cast<PageDirectory*>(mapper.address());
//
//	auto current_virt_addr = mapping->addr();
//
//	for(auto& map : mapping->pages()) {
//#ifdef VMM_LOG_VMAPPING
//		kdebugf("Virt: %x -> Phys %x\n", current_virt_addr, map->address());
//#endif
//
//		auto& pde = dir->get_entry((uint32_t*)current_virt_addr);
//		auto& table = VMM::ensure_page_table(pde);
//		auto& page = table.get_page((uint32_t*)current_virt_addr);
//
//		pde.set_flag(DirectoryFlag::Present, true);
//		pde.set_flag(DirectoryFlag::User, access == MappingPrivilege::UserMode);
//		pde.set_flag(DirectoryFlag::RW, (mapping->flags() & PROT_WRITE));
//
//		page.set_physical((uintptr_t*)(map->address()));
//		page.set_flag(PageFlag::Present, true);
//		page.set_flag(PageFlag::User, access == MappingPrivilege::UserMode);
//		page.set_flag(PageFlag::RW, (mapping->flags() & PROT_WRITE));
//
//		current_virt_addr = (void*)((uint64_t)current_virt_addr + 4096);
//	}
//
//	process->m_maps.push_back(mapping);
//}
//
//void VMM::notify_free_VMapping(VMapping& mapping) {
//	ASSERT_IRQ_DISABLED();
//#ifdef VMM_LOG_VMAPPING
//	kdebugf("[VMM] Freeing VMapping(%x)\n", &mapping);
//#endif
//
//	//  FIXME: Actually delete the mapping
//}
//
//PageTable& VMM::ensure_page_table(PageDirectoryEntry& pde) {
//	if(pde.get_table())
//		return *pde.get_table();
//
//	//  FIXME:  Nasty
//	auto* table = reinterpret_cast<PageTable*>(KMalloc::get().kmalloc_alloc(4096, 0x1000));
//	if(!table)
//		kpanic();
//	memset(table, 0x0, 4096);
//	pde.set_table(reinterpret_cast<PageTable*>(TO_PHYS(table)));
//
//	return *table;
//}
//
//void* VMM::allocate_interrupt_stack() {
//	auto* stack_top = (void*)(0xdd000000);
//	auto& mapping = VMapping::create_for_kernel(stack_top, 4096*4, PROT_READ | PROT_WRITE, MAP_PRIVATE);
//	return (void*)((uint32_t)stack_top+4096*4 - 0);
//}
//
//void* VMM::allocate_kerneltask_stack() {
//	auto* stack_top = (void*)(0xde000000);
//	auto& mapping = VMapping::create_for_kernel(stack_top, 4096*2, PROT_READ | PROT_WRITE, MAP_PRIVATE);
//	return (void*)((uint32_t)stack_top+4096*2 - 0);
//}