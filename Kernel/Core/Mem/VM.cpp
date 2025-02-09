#include <Arch/VM.hpp>
#include <Core/Assert/Panic.hpp>
#include <Core/Error/Error.hpp>
#include <Core/Mem/GFP.hpp>
#include <Core/Mem/Layout.hpp>
#include <Core/Mem/VM.hpp>
#include <LibAllocator/Arena.hpp>
#include <LibAllocator/BumpAllocator.hpp>
#include <LibGeneric/LockGuard.hpp>
#include <LibGeneric/Spinlock.hpp>
#include <SystemTypes.hpp>

//  Protects all data below
static constinit gen::Spinlock s_lock;
//  Root paging handle
//  Modifications to the *kernel* part of the paging tables of the below
//  handle are assumed to be propagated to all children (all running tasks).
static constinit arch::PagingHandle s_root;
//  Bump allocator for vmalloc
//  Currently, no deallocation is possible
//  WARNING: Due to KERNEL_VM_VMALLOC_* being linker symbols, below cannot be constinit!
static liballoc::BumpAllocator s_vmalloc {
	liballoc::Arena { KERNEL_VM_VMALLOC_BASE, KERNEL_VM_VMALLOC_LEN }
};

static void vm_map_kernel(arch::PagingHandle handle) {
	auto* const kernel_elf_start = reinterpret_cast<uint8*>(KERNEL_VM_ELF_BASE);
	auto* const kernel_elf_end = kernel_elf_start + KERNEL_VM_ELF_LEN;
	auto* const kernel_text_start = reinterpret_cast<uint8*>(KERNEL_VM_TEXT_BASE);
	auto* const kernel_text_end = kernel_text_start + KERNEL_VM_TEXT_LEN;

	auto kernel_physical = PhysAddr { KERNEL_PM_LOAD_BASE };
	for(auto addr = kernel_elf_start; addr < kernel_elf_end; addr += 0x1000) {
		auto flags = arch::PageFlags::Read | arch::PageFlags::Write;
		bool in_text_section = (addr >= kernel_text_start && addr < kernel_text_end);
		if(in_text_section) {
			flags = flags | arch::PageFlags::Execute;
		}
		auto err = arch::addrmap(handle, kernel_physical.get(), addr, flags);
		if(err != core::Error::Ok) {
			core::panic("vm_map_kernel failed: addrmap failed!");
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

static void vm_map_identity(arch::PagingHandle handle) {
	auto* const identity_start = reinterpret_cast<uint8_t*>(KERNEL_VM_IDENTITY_BASE);
	auto* const physical_end = get_physical_end();
	if(reinterpret_cast<uintptr_t>(physical_end) > KERNEL_VM_IDENTITY_LEN) {
		core::panic(
		        "vm_map_identity failed: insufficient identity map address space to fit all available physical memory in identity map!");
	}

	auto physical = PhysAddr { nullptr };
	for(auto addr = identity_start; addr < identity_start + (uintptr_t)physical_end; addr += 2_MiB) {
		const auto err = arch::addrmap(handle, physical.get(), addr,
		                               arch::PageFlags::Read | arch::PageFlags::Write | arch::PageFlags::Large);
		if(err != core::Error::Ok) {
			core::panic("vm_map_identity failed: addrmap failed!");
		}
		physical += 2_MiB;
	}
}

//  Create the root paging table
//  This is only done once, when the root paging handle is requested for
//  the first time.
static arch::PagingHandle vm_create_root() {
	auto maybe_handle = arch::addralloc();
	if(!maybe_handle.has_value()) {
		core::panic("Failed to create vmroot: addralloc failed!");
	}
	auto handle = maybe_handle.destructively_move_data();

	vm_map_kernel(handle);
	vm_map_identity(handle);

	return handle;
}

void* core::mem::vmalloc(size_t size) {
	gen::LockGuard lg { s_lock };
	if(!s_root) {
		s_root = vm_create_root();
		if(!s_root) {
			return nullptr;
		}
	}

	const auto actual_allocation_size = ((size + 0x1000 - 1) / 0x1000) * 0x1000;
	auto* base = s_vmalloc.allocate(actual_allocation_size);
	if(!base) {
		return nullptr;
	}

	const auto pages = actual_allocation_size / 0x1000;
	for(size_t i = 0; i < pages; ++i) {
		auto maybe_page = core::mem::allocate_pages(0, {});
		if(!maybe_page) {
			vfree(base);
			return nullptr;
		}
		auto page = maybe_page.destructively_move_data();

		auto* vptr = reinterpret_cast<uint8*>(base) + i * 0x1000;
		const auto err = arch::addrmap(s_root, page.base, vptr, arch::PageFlags::Read | arch::PageFlags::Write);
		if(err != Error::Ok) {
			vfree(base);
			return nullptr;
		}
	}
	return base;
}

void core::mem::vfree(void*) {
	gen::LockGuard lg { s_lock };
	if(!s_root) {
		return;
	}
	//  TODO: Implement deallocation
}

arch::PagingHandle core::mem::get_vmroot() {
	gen::LockGuard lg { s_lock };
	if(!s_root) {
		s_root = vm_create_root();
		if(!s_root) {
			return nullptr;
		}
	}
	return s_root;
}
