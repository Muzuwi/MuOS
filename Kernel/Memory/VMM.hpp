#pragma once
#include <LibGeneric/List.hpp>
#include <Kernel/KOptional.hpp>
#include <Kernel/Memory/VMapping.hpp>
#include <Kernel/Process/Thread.hpp>

using gen::List;

class VMM {
	friend class SlabAllocator;
	friend class Scheduler;

	Process& m_process;
	PhysPtr<PML4> m_pml4;
	List<SharedPtr<VMapping>> m_mappings;
	List<PAllocation> m_kernel_pages;
	void* m_next_kernel_stack_at;   //  FIXME/SMP: Lock this
	void* m_next_anon_vm_at;

	enum class LeakAllocatedPage {
		No,
		Yes
	};

	void _map_pallocation(PAllocation, void*);
	void _map_kernel_executable();
	void _map_physical_identity();
	void _map_kernel_prealloc_pml4();
	KOptional<PhysAddr> _allocate_kernel_page(size_t order);

	KOptional<PhysPtr<PML4>> clone_pml4(PhysPtr<PML4>);
	KOptional<PhysPtr<PDPT>> clone_pdpt(PhysPtr<PDPT>);
	KOptional<PhysPtr<PD>>   clone_pd(PhysPtr<PD>);
	KOptional<PhysPtr<PT>>   clone_pt(PhysPtr<PT>);

	PDPTE* ensure_pdpt(void* addr, LeakAllocatedPage);
	PDE*   ensure_pd(void* addr, LeakAllocatedPage);
	PTE*   ensure_pt(void* addr, LeakAllocatedPage);

	bool addrmap(void* vaddr, PhysAddr, VMappingFlags flags);
	bool addrunmap(void* vaddr);
	bool map(VMapping const&);
	bool unmap(VMapping const&);
public:
	explicit VMM(Process& proc) noexcept
	: m_process(proc), m_pml4(), m_next_kernel_stack_at(&_ukernel_virt_kstack_start), m_next_anon_vm_at(&_userspace_heap_start) {}

	PhysPtr<PML4> pml4() const { return m_pml4; }

	KOptional<SharedPtr<VMapping>> find_vmapping(void* vaddr) const;
	[[nodiscard]] bool insert_vmapping(SharedPtr<VMapping>&&);

	void* allocate_user_stack(uint64 stack_size);
	VMapping* allocate_kernel_stack(uint64 stack_size);

	void* allocate_user_heap(size_t region_size);

	static void initialize_kernel_vm();
	static constexpr unsigned kernel_stack_size() { return 0x4000; }
	static constexpr unsigned user_stack_size() { return 0x4000; }
};