#include <string.h>
#include <Arch/i386/CPUID.hpp>
#include <Arch/i386/Paging.hpp>
#include <Kernel/Debug/kdebugf.hpp>
#include <Kernel/Debug/kpanic.hpp>
#include <Kernel/Memory/PMM.hpp>
#include <Kernel/Memory/Units.hpp>
#include <Kernel/Memory/VMM.hpp>
#include <Kernel/Memory/VMapping.hpp>
#include <Kernel/Process/Process.hpp>
#include <Kernel/Symbols.hpp>

using Units::MiB;
using Units::GiB;

static PhysPtr<PML4> s_kernel_pml4 {nullptr};

void VMM::init() {
	kdebugf("[VMM] Init kernel PML4\n");

	auto res = PMM::allocate();
	if (!res.has_value()) {
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

	kdebugf("[VMM] Preallocating kernel PML4E\n");
	prealloc_kernel_pml4e();

	asm volatile(
	"mov %%rax, %0\n"
	"mov cr3, %%rax\n"
	:
	:""(s_kernel_pml4.get())
	:"rax"
	);
}

void VMM::map_kernel_executable() {
	const auto kernel_elf_start = reinterpret_cast<uint8_t*>(&_ukernel_elf_start);
	const auto kernel_elf_end = reinterpret_cast<uint8_t*>(&_ukernel_elf_end);
	const auto kernel_text_start = reinterpret_cast<uint8_t*>(&_ukernel_text_start);
	const auto kernel_text_end = reinterpret_cast<uint8_t*>(&_ukernel_text_end);

	auto kernel_physical = PhysAddr {&_ukernel_physical_start};
	for (auto addr = kernel_elf_start; addr < kernel_elf_end; addr += 0x1000) {
		auto& pml4e = (*s_kernel_pml4)[addr];
		auto& pdpte = ensure_pdpt(s_kernel_pml4, addr, nullptr);
		auto& pde = ensure_pd(s_kernel_pml4, addr, nullptr);
		auto& pte = ensure_pt(s_kernel_pml4, addr, nullptr);

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

		if (CPUID::has_NXE()) {
			//  Execute only in text sections
			bool xd = !(addr >= kernel_text_start && addr < kernel_text_end);
			pte.set(FlagPTE::ExecuteDisable, xd);
		}

		kernel_physical += 4096;
	}
}

void VMM::map_physical_identity() {
	auto identity_start = reinterpret_cast<uint8_t*>(&_ukernel_identity_start);
	auto physical = PhysAddr {nullptr};

	if (CPUID::has_huge_pages()) {
		for (auto addr = identity_start; addr < identity_start + 512 * GiB; addr += 1 * GiB) {
			auto& pml4e = (*s_kernel_pml4)[addr];
			auto& pdpte = ensure_pdpt(s_kernel_pml4, addr, nullptr);

			pml4e.set(FlagPML4E::Present, true);
			pml4e.set(FlagPML4E::User, false);
			pdpte.set(FlagPDPTE::Present, true);
			pdpte.set(FlagPDPTE::User, false);
			pdpte.set(FlagPDPTE::HugePage, true);
			pdpte.set_directory(physical.as<PD>());

			physical += 1 * GiB;
		}
	} else {
		//  FIXME: Assuming support for 2MiB pages
		//  FIXME: Hard limit on supported physical memory
		for (auto addr = identity_start; addr < identity_start + 512 * GiB; addr += 2 * MiB) {
			auto& pml4e = (*s_kernel_pml4)[addr];
			auto& pdpte = ensure_pdpt(s_kernel_pml4, addr, nullptr);
			auto& pde = ensure_pd(s_kernel_pml4, addr, nullptr);

			pml4e.set(FlagPML4E::Present, true);
			pml4e.set(FlagPML4E::User, false);
			pdpte.set(FlagPDPTE::Present, true);
			pdpte.set(FlagPDPTE::User, false);
			pde.set(FlagPDE::Present, true);
			pde.set(FlagPDE::User, false);
			pde.set(FlagPDE::LargePage, true);
			pde.set_table(physical.as<PT>());

			physical += 2 * MiB;
		}
	}
}

/*
 *  Maps the given PAllocation starting at virtual address vaddr
 *  Should only be used by lower level kernel allocators, userland should use VMappings
 */
void VMM::map_pallocation(PAllocation allocation, void* vaddr) {
	auto physical = allocation.base();
	while (physical < allocation.end()) {
		auto& pml4e = (*s_kernel_pml4)[vaddr];
		auto& pdpte = ensure_pdpt(s_kernel_pml4, vaddr, nullptr);
		auto& pde = ensure_pd(s_kernel_pml4, vaddr, nullptr);
		auto& pte = ensure_pt(s_kernel_pml4, vaddr, nullptr);

		pml4e.set(FlagPML4E::Present, true);
		pdpte.set(FlagPDPTE::Present, true);
		pde.set(FlagPDE::Present, true);
		pte.set(FlagPTE::Present, true);
		pte.set(FlagPTE::Global, true);
		pte.set_page(physical);

		physical += 0x1000;
	}
}

PhysPtr<PML4> VMM::kernel_pml4() {
	return s_kernel_pml4;
}

/*
 *  Maps a given VMapping in the target process
 */
void VMM::vmapping_map(Process* process, VMapping const& mapping) {
	auto pml4 = process->pml4();

	auto virtual_addr = (uint8_t*) mapping.addr();
	for (auto& page : mapping.pages()) {
		auto phys = page.base();
		for (unsigned i = 0; i < (1u << page.order()); ++i) {
			auto& pml4e = (*pml4)[virtual_addr];
			auto& pdpte = ensure_pdpt(pml4, virtual_addr, process->memory());
			auto& pde = ensure_pd(pml4, virtual_addr, process->memory());
			auto& pte = ensure_pt(pml4, virtual_addr, process->memory());

			auto flags = mapping.flags();

			pml4e.set(FlagPML4E::Present, true);
			pml4e.set(FlagPML4E::User, !(flags & VM_KERNEL));
			pml4e.set(FlagPML4E::RW, flags & VM_WRITE);

			pdpte.set(FlagPDPTE::Present, true);
			pdpte.set(FlagPDPTE::User, !(flags & VM_KERNEL));
			pdpte.set(FlagPDPTE::RW, flags & VM_WRITE);

			pde.set(FlagPDE::Present, true);
			pde.set(FlagPDE::User, !(flags & VM_KERNEL));
			pde.set(FlagPDE::RW, flags & VM_WRITE);

			pte.set(FlagPTE::Present, flags & VM_READ);
			pte.set(FlagPTE::User, !(flags & VM_KERNEL));
			pte.set(FlagPTE::ExecuteDisable, !(flags & VM_EXEC));
			pte.set_page(phys);
			pte.set(FlagPTE::RW, (flags & VM_READ) && (flags & VM_WRITE));

//			kdebugf("%x%x -> %x%x\n", (uintptr_t)phys.get()>>32u, (uintptr_t)phys.get()&0xffffffffu, (uintptr_t)virtual_addr>>32u, (uintptr_t)virtual_addr&0xffffffffu);
			virtual_addr += 0x1000;
			phys += 0x1000;
		}
	}
}


/*
 *  Called when a VMapping for a given process is unmapped from it
 */
void VMM::vmapping_unmap(Process*, const VMapping&) {
	//  FIXME:
	kpanic();
}


PDPTE& VMM::ensure_pdpt(PhysPtr<PML4> pml4, void* addr, ProcMem* pm) {
	auto& pml4e = (*pml4)[addr];
	if (!pml4e.directory()) {
		KOptional<PAllocation> alloc;
		if (pm)
			alloc = pm->allocate_phys_kernel(0);
		else
			alloc = PMM::allocate();

		if (!alloc.has_value()) {
			kerrorf("[VMM] Failed to allocate page for PDPT!\n");
			kpanic();
		}
		memset(alloc.unwrap().base().get_mapped(), 0x0, 0x1000);
		pml4e.set_directory(alloc.unwrap().base().as<PDPT>());
	}

	return (*pml4e.directory())[addr];
}


PDE& VMM::ensure_pd(PhysPtr<PML4> pml4, void* addr, ProcMem* pm) {
	auto& pdpte = ensure_pdpt(pml4, addr, pm);
	if (!pdpte.directory()) {
		KOptional<PAllocation> alloc;
		if (pm)
			alloc = pm->allocate_phys_kernel(0);
		else
			alloc = PMM::allocate();

		if (!alloc.has_value()) {
			kerrorf("[VMM] Failed to allocate page for PD!\n");
			kpanic();
		}
		memset(alloc.unwrap().base().get_mapped(), 0x0, 0x1000);
		pdpte.set_directory(alloc.unwrap().base().as<PD>());
	}

	return (*pdpte.directory())[addr];
}


PTE& VMM::ensure_pt(PhysPtr<PML4> pml4, void* addr, ProcMem* pm) {
	auto& pde = ensure_pd(pml4, addr, pm);
	if (!pde.table()) {
		KOptional<PAllocation> alloc;
		if (pm)
			alloc = pm->allocate_phys_kernel(0);
		else
			alloc = PMM::allocate();

		if (!alloc.has_value()) {
			kerrorf("[VMM] Failed to allocate page for PT!\n");
			kpanic();
		}

		memset(alloc.unwrap().base().get_mapped(), 0x0, 0x1000);
		pde.set_table(alloc.unwrap().base().as<PT>());
	}

	return (*pde.table())[addr];
}

/*
 *  Preallocate all PML4 PDPT nodes for the kernel shared address space
 *  That way, any changes to the shared kernel address space will persist throughout all processes,
 *  as all process PML4's are clones of the root PML4
 */
void VMM::prealloc_kernel_pml4e() {
	for(unsigned i = index_pml4e(&_ukernel_shared_start); i <= index_pml4e(&_ukernel_shared_end); ++i) {
		auto addr = (void*)((uint64_t)i << 39ul);
		(*s_kernel_pml4)[addr].set(FlagPML4E::Present, true);
		ensure_pdpt(s_kernel_pml4, addr, nullptr);
	}
}
