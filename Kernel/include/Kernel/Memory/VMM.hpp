#pragma once
#include <Kernel/SystemTypes.hpp>
#include <LibGeneric/SharedPtr.hpp>
#include <Arch/i386/Paging.hpp>
#include <Kernel/Memory/PMM.hpp>

class Process;
class VMapping;

class VMM final {
	friend class VMapping;
	friend class SlabAllocator;

	template<class T>
	using SharedPtr = gen::SharedPtr<T>;

	static void vmapping_map(Process*, VMapping const&);
	static void vmapping_unmap(Process*, VMapping const&);

	static PDPTE& ensure_pdpt(PhysPtr<PML4>, void* addr, ProcMem*);
	static PDE& ensure_pd(PhysPtr<PML4>, void* addr, ProcMem*);
	static PTE& ensure_pt(PhysPtr<PML4>, void* addr, ProcMem*);

	static void map_kernel_executable();
	static void map_physical_identity();
	static void prealloc_kernel_pml4e();
	static void map_pallocation(PAllocation, void*);
public:
	static void init();
	static PhysPtr<PML4> kernel_pml4();

	static constexpr unsigned kernel_stack_size() {
		return 0x4000;
	}

	static constexpr unsigned user_stack_size() {
		return 0x4000;
	}
};
