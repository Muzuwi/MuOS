#include <Arch/riscv64/Boot0/Boot0.hpp>
#include <Arch/riscv64/Boot0/BootConsole.hpp>
#include <Arch/riscv64/Boot0/Memory.hpp>
#include <Arch/riscv64/Boot0/TinyMM.hpp>
#include <LibAllocator/Arena.hpp>
#include <LibAllocator/BumpAllocator.hpp>
#include <LibFDT/DeviceTree.hpp>
#include <LibGeneric/Allocator.hpp>
#include <string.h>
#include <SystemTypes.hpp>

///  Copy of the kernel header in memory to boot from
static KernelImageHeader s_kernel_header;
///  Page allocator
static liballoc::BumpAllocator s_phys_allocator {
	liballoc::Arena { nullptr, 0 }
};

[[noreturn]] static void boot0_panic(char const* reason) {
	bootcon::printf(LOGPFX "Panic called with reason: {}\n", reason);
	bootcon::printf(LOGPFX "Boot aborted, system halted.\n");
	while(true) {
		//  hang
	}
}

static bool cb_check_props(libfdt::FdtHeader const* header, libfdt::FdtNodeHandle, libfdt::FdtPropHandle phandle,
                           void*) {
	bootcon::printf("\t{} ", libfdt::prop_name(header, phandle));

	if(libfdt::prop_len(header, phandle) > 0) {
		bootcon::printf("{} ", libfdt::prop_read_string(header, phandle));
	}

	uint32 val;
	if(libfdt::prop_read_u32(header, phandle, &val)) {
		bootcon::printf("{x} ", val);
	}

	uint64 val64;
	if(libfdt::prop_read_u64(header, phandle, &val64)) {
		bootcon::printf("{x} ", val64);
	}
	bootcon::putch(' ');
	if(libfdt::prop_read_u64(header, phandle, &val64, 1)) {
		bootcon::printf("{x} ", val64);
	}

	bootcon::printf("\n");
	return true;
}

static bool cb_check_memory_node(libfdt::FdtHeader const* header, libfdt::FdtNodeHandle handle, void* args) {
	bootcon::printf("{}\n", libfdt::node_name(header, handle));
	libfdt::visit_each_prop(header, handle, cb_check_props, args);
	return true;
}

///  Extract usable memory from the device tree and update tinymm mappings.
static void mm_parse_usable(libfdt::FdtHeader const* handle) {
	libfdt::FdtNodeHandle memory = libfdt::find_first_node_by_unit_name(handle, "memory");
	if(!memory) {
		boot0_panic("Could not find 'memory' node in FDT");
	}

	auto reg = libfdt::node_get_named_prop(handle, memory, "reg");
	if(!reg) {
		boot0_panic("Could not get 'reg' property of 'memory' node");
	}

	libfdt::visit_each_node(handle, cb_check_memory_node, nullptr);

	//  TODO: Assumes address_cells=2, size_cells=2
	const auto len = libfdt::prop_len(handle, reg);
	const auto ranges = len / (4 * 4);
	for(size_t i = 0; i < ranges; ++i) {
		uint64 address;
		uint64 size;

		if(!libfdt::prop_read_u64(handle, reg, &address, i * 2 + 0)) {
			boot0_panic("Could not get cell from prop");
		}
		if(!libfdt::prop_read_u64(handle, reg, &size, i * 2 + 1)) {
			boot0_panic("Could not get cell from prop");
		}

		tinymm::update_region((void*)address, size, tinymm::RegionType::Usable);
	}
}

///  Extract reserved memory from the device tree and update tinymm mappings.
static void mm_parse_reserved(libfdt::FdtHeader const* handle) {
	libfdt::FdtNodeHandle reserved = libfdt::find_first_node_by_unit_name(handle, "reserved-memory");
	if(!reserved) {
		bootcon::printf(LOGPFX, "No reserved-memory nodes found");
		return;
	}

	//  TODO: Assumes address_cells=2, size_cells=2
	libfdt::FdtNodeHandle child = nullptr;
	while(libfdt::find_next_child(handle, reserved, &child)) {
		auto reg = libfdt::node_get_named_prop(handle, child, "reg");
		if(!reg) {
			bootcon::printf(LOGPFX "warn: Could not get 'reg' property of 'memory-reserved' child node\n");
			continue;
		}

		uint64 address;
		uint64 size;
		if(!libfdt::prop_read_u64(handle, reg, &address, 0)) {
			bootcon::printf(LOGPFX "warn: Could not get address cell from reserved memory\n");
			continue;
		}
		if(!libfdt::prop_read_u64(handle, reg, &size, 1)) {
			bootcon::printf(LOGPFX "warn: Could not get size cell from reserved memory\n");
			continue;
		}

		tinymm::update_region((void*)address, size, tinymm::RegionType::ReservedByHardware);
	}
}

///  Reserve space that was taken by boot0 in the tinymm memory map
///  This needs to be done, as otherwise we'd attempt to allocate from
///  memory taken by boot0 itself.
static void mm_reserve_boot0() {
	const auto len = &__BOOT0_PHYS_END - &__BOOT0_PHYS_BASE;
	tinymm::update_region((void*)&__BOOT0_PHYS_BASE, len, tinymm::RegionType::ReservedByHardware);
}

///  Dump the current tinymm memory map
static void mm_dump() {
	bootcon::printf(LOGPFX "Memory map:\n");
	size_t count;
	auto* regions = tinymm::regions(&count);
	if(!regions) {
		boot0_panic("tinymm failed");
	}

	for(size_t i = 0; i < count; ++i) {
		auto const& region = regions[i];
		const auto start = (uint64)region.start;
		const auto end = (uint64)region.start + (uint64)region.len - 1ull;

		auto* type_str = [region]() -> char const* {
			switch(region.type) {
				case tinymm::RegionType::Usable: {
					return "Usable";
				}
				case tinymm::RegionType::ReservedByHardware: {
					return "Reserved";
				}
				case tinymm::RegionType::Allocator: {
					return "Allocator";
				}
				default: return "<INVALID>";
			}
		}();
		bootcon::printf(" - {x}:{x} [{}]\n", start, end, type_str);
	}
}

void addrmap(void* paddr, void* vaddr, PageTableEntryFlags flags) {
#define NEXT(entry) ((uint64*)((entry & NEXT_LEVEL_MASK) << NEXT_LEVEL_SHIFT))

	//  Accessed/Dirty bits MUST be pre-populated while we're in boot0.
	//  We cannot assume that the hardware supports Svadu, which would
	//  do that for us automatically.
	const auto entry_flags = PTE_VALID | PTE_ACCESSED | PTE_DIRTY | flags;

	if((flags & VM_2M) && (!IS_POW2_ALIGNED(PAGE_SIZE_2M, paddr) || !IS_POW2_ALIGNED(PAGE_SIZE_2M, vaddr))) {
		boot0_panic("Unaligned pointer passed to addrmap");
	}
	if((flags & VM_1G) && (!IS_POW2_ALIGNED(PAGE_SIZE_1G, paddr) || !IS_POW2_ALIGNED(PAGE_SIZE_1G, vaddr))) {
		boot0_panic("Unaligned pointer passed to addrmap");
	}

	auto* satp = (uint64*)read_csr(satp);
	//  Allocate top-level structure if it is not initialized yet
	if(!satp) {
		satp = (uint64*)s_phys_allocator.allocate(PAGE_SIZE);
		if(!satp) {
			boot0_panic("Failed to allocate page for addrmap, out of memory?");
		}
		mem::memset(satp, 0x0, PAGE_SIZE);
		//  Write the top-level pointer, but don't enable paging yet
		write_csr(satp, (uint64)satp / 0x1000);
	} else {
		satp = (uint64*)((uint64)satp * 0x1000);
	}

	//  VPN2
	auto& vpn2e = satp[VPN2((uint64)vaddr)];
	if(!(vpn2e & PTE_VALID)) {
		if(flags & VM_1G) {
			vpn2e = (PPN2((uint64)paddr) << 28) | (PPN1((uint64)paddr) << 19) | (PPN0((uint64)paddr) << 10) |
			        entry_flags;
			return;
		}

		auto* l2ptr = (uint64*)s_phys_allocator.allocate(PAGE_SIZE);
		if(!l2ptr) {
			boot0_panic("Failed to allocate page for addrmap, out of memory?");
		}
		mem::memset(l2ptr, 0x0, PAGE_SIZE);
		vpn2e = (PPN2((uint64)l2ptr) << 28) | (PPN1((uint64)l2ptr) << 19) | (PPN0((uint64)l2ptr) << 10) | PTE_VALID;
	}

	auto& vpn1e = NEXT(vpn2e)[VPN1((uint64)vaddr)];
	if(!(vpn1e & PTE_VALID)) {
		if(flags & VM_2M) {
			vpn1e = (PPN2((uint64)paddr) << 28) | (PPN1((uint64)paddr) << 19) | (PPN0((uint64)paddr) << 10) |
			        entry_flags;
			return;
		}

		auto* l1ptr = (uint64*)s_phys_allocator.allocate(PAGE_SIZE);
		if(!l1ptr) {
			boot0_panic("Failed to allocate page for addrmap, out of memory?");
		}
		mem::memset(l1ptr, 0x0, PAGE_SIZE);
		vpn1e = (PPN2((uint64)l1ptr) << 28) | (PPN1((uint64)l1ptr) << 19) | (PPN0((uint64)l1ptr) << 10) | PTE_VALID;
	}

	auto& vpn0e = NEXT(vpn1e)[VPN0((uint64)vaddr)];
	vpn0e = (PPN2((uint64)paddr) << 28) | (PPN1((uint64)paddr) << 19) | (PPN0((uint64)paddr) << 10) | entry_flags;
}

///  Pick the first available physical memory region to use as allocator
///  storage for boot0. Note that the whole region isn't used but only
///  a part of it. This must be called before trying to map things into
///  the virtual address space.
static void init_physical_allocator() {
	size_t count;
	auto const* regions = tinymm::regions(&count);
	if(!regions) {
		boot0_panic("tinymm failed");
	}

	tinymm::Region const* region = nullptr;
	for(size_t i = 0; i < count; ++i) {
		if(regions[i].type != tinymm::RegionType::Usable) {
			continue;
		}
		region = &regions[i];
		break;
	}
	if(region == nullptr) {
		boot0_panic("Cannot find usable region for VM mappings");
	}

	bootcon::printf(LOGPFX "Preloader will use memory starting from physical address: {}\n", region->start);

	new(&s_phys_allocator) liballoc::BumpAllocator {
		liballoc::Arena { region->start, region->len }
	};
}

///  Identity map boot0 in the prepared bootstrap mappings.
///  Otherwise, boot0 would crash immediately after enabling paging.
static void vm_map_boot0() {
	bootcon::printf(LOGPFX "Mapping kernel loader address spaces..\n");
	for(auto* addr = (uint8*)&__BOOT0_PHYS_BASE; addr < &__BOOT0_PHYS_END; addr += PAGE_SIZE) {
		addrmap((void*)addr, (void*)addr, DEFAULT_FLAGS);
	}
}

///  Create the kernel physical memory map in the bootstrap mappings
static void vm_map_physical() {
	size_t count;
	auto const* regions = tinymm::regions(&count);
	if(!regions) {
		boot0_panic("tinymm failed");
	}
	auto* physical_identity_vm_start = reinterpret_cast<uint8*>(0xFFFFFFD000000000);

	bootcon::printf(LOGPFX "Mapping physical memory..\n");
	auto* ppmax = reinterpret_cast<uint8*>(regions[count - 1].start) + regions[count - 1].len;
	const auto huge_count = (reinterpret_cast<uintptr_t>(ppmax) + PAGE_SIZE_1G - 1) / PAGE_SIZE_1G;
	for(size_t i = 0; i < huge_count; ++i) {
		auto* pptr = reinterpret_cast<uint8*>(i * PAGE_SIZE_1G);
		auto* vptr = physical_identity_vm_start + reinterpret_cast<uintptr_t>(pptr);
		addrmap(pptr, vptr, (PageTableEntryFlags)(DEFAULT_FLAGS | VM_1G));
	}
}

///  Map the kernel image into virtual memory, as specified in the kernel image header
static void vm_map_kernel_image() {
	void* phys_end = &__BOOT0_PHYS_END;

	const uint64 kernel_image_size = s_kernel_header.vmsize;
	void const* phys_end_kernel = reinterpret_cast<uint8 const*>(phys_end) + kernel_image_size;
	auto const* kernel_vm_start = reinterpret_cast<uint8 const*>(s_kernel_header.vmbase);
	for(auto* addr = (uint8*)phys_end; addr < phys_end_kernel; addr += PAGE_SIZE) {
		//  Map the virtual address space used by the proper kernel
		addrmap((void*)addr, (void*)(kernel_vm_start + (addr - (uint8*)phys_end)), DEFAULT_FLAGS);
	}
}

///  Machine-specific mappings go here
static void vm_map_platform() {
	bootcon::remap();
}

///  Enable Sv39 paging using previously prepared bootstrap mappings
static void vm_switch() {
	bootcon::printf(LOGPFX "Switching to VM..\n");

	const auto satp = read_csr(satp);
	write_csr(satp, satp | (8ul << 60ul));
	asm volatile("sfence.vma" : : : "memory");
}

///  Jump to the kernel in virtual memory and pass the FDT.
[[noreturn]] static void jump_to_vm_kernel(void* fdt) {
	auto* start = reinterpret_cast<uint8*>(s_phys_allocator.start());
	auto* current = reinterpret_cast<uint8*>(s_phys_allocator.current());
	bootcon::printf(LOGPFX "boot0 used: {x} bytes of physical memory\n", current - start);
	bootcon::printf(LOGPFX "Updating tinymm mappings..\n");
	tinymm::update_region((void*)start, current - start, tinymm::RegionType::Allocator);
	mm_dump();

	bootcon::printf(LOGPFX "Jumping to kernel in virtual address space!\n");
	s_kernel_header.vmentry(fdt);
	boot0_panic("VM kernel jump failed");
}

extern "C" void boot0_handle_exception(uint64 scause, uint64 sepc, uint64 stval) {
	bootcon::printf(LOGPFX "Fatal error, caught trap during boot!\n");
	bootcon::printf("SCAUSE={x} SEPC={x} STVAL={x}\n", scause, sepc, stval);
	boot0_panic("Unhandled trap caught");
}

extern "C" void boot0_main(void* arg0, void* sbi_fdt) {
	auto* handle = libfdt::init_parser((uint8 const*)sbi_fdt);
	if(!handle) {
		//  This panic won't print anything as bootcon is not initialized yet, but
		//  if the FDT was invalid then there's nothing much we can do anyway.
		boot0_panic("FDT init failed");
	}
	bootcon::init(handle);

	bootcon::printf(LOGPFX "Reached physical memory entrypoint\n");
	bootcon::printf(LOGPFX "arg0={} arg1={}\n", arg0, sbi_fdt);

	//  Kernel image is expected right after the boot0 binary in physical memory
	auto const* header = reinterpret_cast<KernelImageHeader const*>(&__BOOT0_PHYS_END);
	if(header->magic != KernelImageHeader::MAGIC_VALUE) {
		boot0_panic("Could not find kernel header in memory to boot from");
	}
	mem::memcpy(&s_kernel_header, header, sizeof(KernelImageHeader));
	bootcon::printf(LOGPFX "Found bootable kernel header at {}\n", Format::ptr(header));
	bootcon::printf(LOGPFX "   vmbase={}\n", header->vmbase);
	bootcon::printf(LOGPFX "   vmsize={x}\n", header->vmsize);
	bootcon::printf(LOGPFX "   vmentry={}\n", Format::ptr(header->vmentry));

	mm_parse_usable(handle);
	mm_parse_reserved(handle);
	mm_reserve_boot0();
	mm_dump();
	init_physical_allocator();
	vm_map_boot0();
	vm_map_physical();
	vm_map_kernel_image();
	vm_map_platform();
	vm_switch();
	jump_to_vm_kernel(sbi_fdt);
}
