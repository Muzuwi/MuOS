#include <Arch/VM.hpp>
#include <Core/Assert/Assert.hpp>
#include <Core/Assert/Panic.hpp>
#include <Core/Error/Error.hpp>
#include <Core/Log/Logger.hpp>
#include <Core/Mem/GFP.hpp>
#include <Core/Mem/Layout.hpp>
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

	log.info("Initializing kerneld address space");
	auto maybe_handle = arch::addralloc();
	if(maybe_handle.has_error()) {
		core::panic("Failed allocating paging handle for kerneld process! Out of memory?");
	}
	kerneld.vmm().m_paging_handle = maybe_handle.data();

	log.debug("Mapping kernel executable");
	kerneld.vmm()._map_kernel_executable();

	log.debug("Creating physical identity map");
	kerneld.vmm()._map_physical_identity();

	asm volatile("mov %%rax, %0\n"
	             "mov cr3, %%rax\n"
	             :
	             : "r"(kerneld.vmm().m_paging_handle)
	             : "rax");
}

/*
 *  Maps the given PAllocation starting at virtual address vaddr
 *  Should only be used by lower level kernel allocators, userland should use VMappings
 */
void VMM::_map_pallocation(core::mem::PageAllocation allocation, void* vaddr) {
	auto physical = PhysAddr { allocation.base };
	while(physical.get() < allocation.end()) {
		auto err =
		        arch::addrmap(m_paging_handle, physical.get(), vaddr, arch::PageFlags::Read | arch::PageFlags::Write);
		if(err != core::Error::Ok) {
			core::panic("Failed to map PageAllocation for kernel allocator!");
		}
		physical += 0x1000;
		vaddr = reinterpret_cast<uint8*>(vaddr) + 0x1000;
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

	auto kernel_physical = PhysAddr { &_ukernel_physical_start };
	for(auto addr = kernel_elf_start; addr < kernel_elf_end; addr += 0x1000) {
		auto flags = arch::PageFlags::Read | arch::PageFlags::Write;
		bool in_text_section = (addr >= kernel_text_start && addr < kernel_text_end);
		if(in_text_section) {
			flags = flags | arch::PageFlags::Execute;
		}
		auto err = arch::addrmap(m_paging_handle, kernel_physical.get(), addr, flags);
		if(err != core::Error::Ok) {
			core::panic("Failed to addrmap kernel executable!");
		}
		kernel_physical += 0x1000;
	}
}

static void* get_physical_end() {
	void* max = nullptr;
	core::mem::for_each_region([&max](core::mem::Region region) {
		if(region.start > max) {
			max = region.start;
		}
	});
	return max;
}

/*
 *  Creates the identity map in the current address space. Only called on kernel
 *  initialization
 */
void VMM::_map_physical_identity() {
	auto identity_start = reinterpret_cast<uint8_t*>(&_ukernel_identity_start);
	auto physical = PhysAddr { nullptr };

	const auto physical_end = get_physical_end();

	for(auto addr = identity_start; addr < identity_start + (uintptr_t)physical_end; addr += 2_MiB) {
		const auto err = arch::addrmap(m_paging_handle, physical.get(), addr,
		                               arch::PageFlags::Read | arch::PageFlags::Write | arch::PageFlags::Large);
		if(err != core::Error::Ok) {
			core::panic("Failed to create physical identity map!");
		}
		physical += 2_MiB;
	}
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
 *  Allocates a physical page for use in the kernel, on behalf of the current process.
 *  After the process is removed, these pages WILL BE FREED.
 *  Can't use VMappings for these, as most often they won't have an underlying VM mapping
 */
KOptional<PhysAddr> VMM::_allocate_kernel_page(size_t order) {
	auto alloc = core::mem::allocate_pages(order, core::mem::PageAllocFlags {});
	if(!alloc.has_value()) {
		return {};
	}

	m_kernel_pages.push_back(alloc.data());
	return { alloc.data().base };
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

VMapping* VMM::allocate_kernel_stack(uint64 stack_size) {
	auto lock = acquire_vm_lock();

	void* kstack_top = m_next_kernel_stack_at;
	void* kstack_bottom = (void*)((uintptr_t)m_next_kernel_stack_at + stack_size);

	//  FIXME/LIMITS
	if(kstack_bottom > &_ukernel_virt_kstack_end) {
		return {};
	}
	//  FIXME: Handle randomize_vm flag

	m_next_kernel_stack_at = (void*)((uintptr_t)m_next_kernel_stack_at + stack_size + 0x1000);

	auto mapping = VMapping::create((void*)kstack_top, stack_size, VM_READ | VM_WRITE | VM_KERNEL, MAP_PRIVATE);
	auto mapping_ptr = mapping.get();
	ENSURE(insert_vmapping(gen::move(mapping)));

	return mapping_ptr;
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

static liballoc::BumpAllocator s_heap_break { liballoc::Arena(
	    &_ukernel_heap_start, (size_t)((uint8*)&_ukernel_heap_end - (uint8*)&_ukernel_heap_start)) };

/*
 *  Allocates a region of the specified size (rounded to integer amount of pages) in the kernel heap address space.
 *  This will automatically allocate pages and map the region in the default kernel address space (kerneld).
 *  This should only be used by low-level kernel allocators!
 */
void* VMM::allocate_kernel_heap(size_t size) {
	ENSURE(size > 0);
	auto size_rounded_to_page_size = ((size + 0x1000 - 1) & ~(0x1000 - 1));

	auto* ptr = s_heap_break.allocate(size_rounded_to_page_size);

	if(ptr == nullptr) {
		return nullptr;
	}

	//  Map new pages for the allocated heap region
	for(auto* current = (uint8*)ptr; current < (uint8*)ptr + size_rounded_to_page_size; current += 0x1000) {
		auto page = core::mem::allocate_pages(0, core::mem::PageAllocFlags {});
		if(!page.has_value()) {
			return nullptr;
		}

		Process::_kerneld_ref().vmm()._map_pallocation(page.data(), current);
	}

	return ptr;
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
