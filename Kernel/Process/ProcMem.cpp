#include <Kernel/Symbols.hpp>
#include <Kernel/Process/ProcMem.hpp>

ProcMem::ProcMem()
: m_kernel_phys(), m_regions() {}

KOptional<VMRegion*> ProcMem::find_vmregion(void* vaddr) {
	for(auto& region : m_regions) {
		if(region.contains(vaddr))
			return KOptional<VMRegion*>{&region};
	}

 	return KOptional<VMRegion*>{};
}

KOptional<VMRegion const*> ProcMem::find_vmregion(void* vaddr) const {
	for(auto& region : m_regions) {
		if(region.contains(vaddr))
			return KOptional<VMRegion const*>{&region};
	}

	return KOptional<VMRegion const*>{};
}

void ProcMem::create_vmregion(gen::SharedPtr<VMapping>&& mapping) {
	auto* addr = mapping->addr();
	auto size = mapping->size();

	auto it = find_vmregion_iterator(addr);
	auto vmregion = m_regions.insert(it, VMRegion{mapping, size});

//	for(auto& v : m_regions) {
//		auto end = (uintptr_t)v.start() + v.size();
//		kdebugf("VMRegion: %x%x - %x%x\n", (uintptr_t)v.start()>>32u, (uintptr_t)v.start()&0xffffffffu, end>>32u, end&0xffffffffu);
//	}
}

ProcMem::VMListIt ProcMem::find_vmregion_iterator(void* vaddr) {
	auto it = m_regions.begin();
	while(it != m_regions.end() && (*it).start() < vaddr)
		++it;

	return it;
}

/*
 *  Allocates a physical page for use in the kernel, on behalf of the current process
 *  Used for proper freeing of pages used for page tables, and prevents having to change
 *  the Process::s_current field when creating a process in a different process
 *  Can't use VMRegion for these, as most often they won't have an underlying VM mapping
 *  Callers are not expected to manually free PAllocation allocated with this function
 */
KOptional<PAllocation> ProcMem::allocate_phys_kernel(size_t alloc_order) {
//	auto* ret = __builtin_return_address(0);
//	kdebugf("Alloc for kernel %i, return for %x%x\n", alloc_order, (uintptr_t)ret>>32u, (uintptr_t)ret&0xffffffffu);
	auto alloc = PMM::allocate(alloc_order);
	if(alloc.has_value())
		m_kernel_phys.push_back(alloc.unwrap());
	return alloc;
}
