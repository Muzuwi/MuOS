#pragma once
#include <Arch/VM.hpp>
#include <Core/Mem/GFP.hpp>
#include <Daemons/SysDbg/SysDbg.hpp>
#include <LibGeneric/List.hpp>
#include <LibGeneric/LockGuard.hpp>
#include <LibGeneric/Spinlock.hpp>
#include <Memory/Wrappers/VMapping.hpp>
#include <Process/Thread.hpp>
#include <Structs/KOptional.hpp>

using gen::List;

class VMM {
	friend void SysDbg::handle_command(gen::List<gen::String> const& args);
	friend class V86;
	friend class SMP;
	friend void SysDbg::dump_process(gen::SharedPtr<Process> process, size_t depth);

	Process& m_process;
	arch::PagingHandle m_paging_handle;
	List<SharedPtr<VMapping>> m_mappings;
	List<core::mem::PageAllocation> m_kernel_pages;
	void* m_next_anon_vm_at;
	gen::Spinlock m_vm_lock;

	enum class LeakAllocatedPage {
		No,
		Yes
	};

	bool map(VMapping const&);
	bool unmap(VMapping const&);
public:
	explicit VMM(Process& proc) noexcept
	    : m_process(proc)
	    , m_next_anon_vm_at(&_userspace_heap_start) {}

	arch::PagingHandle paging_handle() const { return m_paging_handle; }

	KOptional<SharedPtr<VMapping>> find_vmapping(void* vaddr) const;
	[[nodiscard]] bool insert_vmapping(SharedPtr<VMapping>&&);

	gen::LockGuard<gen::Spinlock> acquire_vm_lock();

	void* allocate_user_stack(uint64 stack_size);
	void* allocate_user_heap(size_t region_size);

	bool clone_address_space_from(arch::PagingHandle);

	static void initialize_kernel_vm();
	static constexpr unsigned kernel_stack_size() { return 0x4000; }
	static constexpr unsigned user_stack_size() { return 0x4000; }

	bool addrmap(void* vaddr, PhysAddr, VMappingFlags flags);
	bool addrunmap(void* vaddr);
};
