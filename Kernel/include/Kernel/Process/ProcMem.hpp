#pragma once
#include <LibGeneric/List.hpp>
#include <Kernel/Memory/VMRegion.hpp>

class ProcMem {
	using VMList = gen::List<VMRegion>;
	using VMListIt = gen::List<VMRegion>::iterator;

	gen::List<PAllocation> m_kernel_phys;
	gen::List<VMRegion> m_regions;
	VMListIt find_vmregion_iterator(void* vaddr);
public:
	ProcMem();

	void create_vmregion(gen::SharedPtr<VMapping>&& mapping);
	KOptional<VMRegion*> find_vmregion(void* vaddr);
	KOptional<VMRegion const*> find_vmregion(void* vaddr) const;

	KOptional<PAllocation> allocate_phys_kernel(size_t alloc_order = 0);
};