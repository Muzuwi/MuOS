#include <Arch/VM.hpp>
#include <Core/Assert/Assert.hpp>
#include <Core/Assert/Panic.hpp>
#include <Core/Error/Error.hpp>
#include <Core/Log/Logger.hpp>
#include <Core/Mem/GFP.hpp>
#include <Core/Mem/Layout.hpp>
#include <Core/Mem/VM.hpp>
#include <LibAllocator/BumpAllocator.hpp>
#include <Memory/VMM.hpp>
#include <Process/Process.hpp>
#include <string.h>
#include <SystemTypes.hpp>

CREATE_LOGGER("vmm", core::log::LogLevel::Debug);

/*
 *  Initializes the address space used by the kernel in the kerneld process
 */
void VMM::initialize_kernel_vm() {
	Process& kerneld = Process::_kerneld_ref();

	kerneld.vmm().m_paging_handle = core::mem::get_vmroot();
	asm volatile("mov %%rax, %0\n"
	             "mov cr3, %%rax\n"
	             :
	             : "r"(kerneld.vmm().m_paging_handle)
	             : "rax");
}

/*
 *  Maps a given VMapping in the target process
 */
bool VMM::map(VMapping const& mapping) {
	auto virtual_addr = (uint8_t*)mapping.addr();
	for(auto& page : mapping.pages()) {
		auto phys = PhysAddr { page.base };
		for(unsigned i = 0; i < (1u << page.order); ++i) {
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
		auto phys = PhysAddr { page.base };
		for(unsigned i = 0; i < (1u << page.order); ++i) {
			addrunmap(virtual_addr);
			virtual_addr += 0x1000;
			phys += 0x1000;
		}
	}
	return true;
}

/*
 *  Looks for a VMapping for the given virtual address
 */
KOptional<SharedPtr<VMapping>> VMM::find_vmapping(void* vaddr) const {
	for(auto& vmapping : m_mappings) {
		if(!vmapping) {
			continue;
		}
		if(!vmapping->contains(vaddr)) {
			continue;
		}

		return { vmapping };
	}

	return {};
}

/*
 *  Validates whether the given VMapping does not overlap with any other mappings,
 *  and saves it into the list
 */
bool VMM::insert_vmapping(SharedPtr<VMapping>&& mapping) {
	if(!mapping) {
		return false;
	}

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
	auto lock = acquire_vm_lock();

	//  FIXME: Actually randomize
	auto random = [](size_t, size_t) -> size_t { return 4; };

	uint64_t stack_top;
	if(m_process.flags().randomize_vm) {
		//  FIXME: Use stack_size
		stack_top = (uintptr_t)&_userspace_stack_start + VMM::user_stack_size() * random(0, 0x40000);
	} else {
		stack_top = (uintptr_t)&_userspace_stack_start;
	}

	auto stack_mapping = VMapping::create((void*)stack_top, stack_size, VM_READ | VM_WRITE, MAP_PRIVATE);
	ENSURE(insert_vmapping(gen::move(stack_mapping)));

	return (void*)(stack_top + stack_size);
}

void* VMM::allocate_user_heap(size_t region_size) {
	auto lock = acquire_vm_lock();

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

bool VMM::clone_address_space_from(arch::PagingHandle handle) {
	auto maybe_handle = arch::addrclone(handle);
	if(maybe_handle.has_error()) {
		return false;
	}

	m_paging_handle = maybe_handle.destructively_move_data();
	return true;
}

gen::LockGuard<gen::Spinlock> VMM::acquire_vm_lock() {
	return gen::LockGuard { m_vm_lock };
}

bool VMM::addrmap(void* vaddr, PhysAddr paddr, VMappingFlags flags) {
	arch::PageFlags arch_flags {};
	if(flags & VM_READ) {
		arch_flags = arch_flags | arch::PageFlags::Read;
	}
	if(flags & VM_WRITE) {
		arch_flags = arch_flags | arch::PageFlags::Write;
	}
	if(flags & VM_EXEC) {
		arch_flags = arch_flags | arch::PageFlags::Execute;
	}
	if(!(flags & VM_KERNEL)) {
		arch_flags = arch_flags | arch::PageFlags::User;
	}

	const auto err = arch::addrmap(m_paging_handle, paddr.get(), vaddr, arch_flags);
	return err == core::Error::Ok;
}

bool VMM::addrunmap(void* vaddr) {
	const auto err = arch::addrunmap(m_paging_handle, vaddr);
	return err == core::Error::Ok;
}
