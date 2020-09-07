#include <Kernel/Memory/PMM.hpp>
#include <Kernel/Memory/kmalloc.hpp>
#include <Kernel/Debug/kdebugf.hpp>
#include <Arch/i386/Multiboot.hpp>
#include <Kernel/Symbols.hpp>
#include <Kernel/Memory/PRegion.hpp>
#include <LibGeneric/List.hpp>
#include <Kernel/Memory/PageToken.hpp>
#include <LibGeneric/SharedPtr.hpp>
#include <Kernel/Process/Process.hpp>

//  Amount of physical memory to reserve for kernel data
static const unsigned kernel_reserved = 16 * MiB;

//  Memory regions dedicated to the kernel (1MiB - 16MiB physical)
static gen::List<PRegion*> s_kernel_area;

//  Memory region dedicated to userland (16MiB+)
static gen::List<PRegion*> s_user_area;

void PMM::handle_multiboot_memmap(void* multiboot_mmap) {
	auto mmap_len = ((uintptr_t*)multiboot_mmap)[0];
	auto mmap_addr = (uint32_t)TO_VIRT(((uintptr_t*)multiboot_mmap)[1]);

#ifdef LEAKY_LOG
	kdebugf("[PMM] mmap size: %x\n", mmap_len);
	kdebugf("[PMM] mmap addr: %x\n", mmap_addr);
#endif

	kdebugf("[PMM] Multiboot memory map:\n");

	//  Total system memory
	uint64_t memory_amount = 0, reserved_amount = 0;

	uint32_t pointer = mmap_addr;
	while(pointer < mmap_addr + mmap_len){
		uint32_t size  = *((uint32_t*)pointer);
		uint64_t start = *((uint64_t*)(pointer + 4));
		uint64_t range = *((uint64_t*)(pointer + 12));
		uint64_t end   = start + range;
		auto type = *((uint32_t*)(pointer + 20));
		kdebugf("[PMM] %x%x", (uint32_t)((start >> 32) & (0xFFFFFFFF)),(uint32_t)(start & 0xFFFFFFFF));
		kdebugf(" - %x%x: ", (uint32_t)((end >> 32) & (0xFFFFFFFF)),(uint32_t)(end & 0xFFFFFFFF));

		switch((mmap_memory_type_t)type){
			case USABLE:
				kdebugf("usable\n");
				memory_amount += range;

				//  FIXME:
				if(end < 1 * MiB)
					break;
				if(end > 0xffffffff) {
					kdebugf("unaddressable, ignoring\n");
					break;
				}

				if(start < kernel_reserved) {
					//  Split the region, if on the boundary
					if(end > kernel_reserved) {
						size_t size_kernel = kernel_reserved - start;
						size_t size_user   = end - kernel_reserved;

						s_kernel_area.push_back(new PRegion(start, size_kernel));
						s_user_area.push_back(new PRegion(kernel_reserved, size_user));
					} else {
						s_kernel_area.push_back(new PRegion(start, range));
					}
				} else {
					s_user_area.push_back(new PRegion(start, range));
				}

				break;

			case HIBERN:
				kdebugf("to be preserved\n");
				break;

			case ACPI:
				kdebugf("acpi\n");
				break;

			case BAD:
				kdebugf("defective\n");
				break;

			default:
				kdebugf("reserved\n");
				reserved_amount += range;
				break;
		}

		pointer += size + 4;
	}

    uint32_t kernel_start = (uint32_t)(&_ukernel_start),
             kernel_end   = (uint32_t)(&_ukernel_end);

	uint32_t mem_mib = memory_amount / 0x100000;

	kdebugf("[PMM] Kernel-used memory: %i MiB\n", (kernel_end - kernel_start) / 0x100000);
	kdebugf("[PMM] Total usable memory: %i MiB\n", mem_mib);
	kdebugf("[PMM] Reserved memory: %i bytes\n", reserved_amount);
	kdebugf("[PMM] Kernel-reserved regions: %i\n", s_kernel_area.size());
#ifndef LEAKY_LOG
	for(auto& reg : s_kernel_area) {
		kdebugf("  - PRegion(%x): start %x, size %i\n", reg, reg->addr(), reg->size());
	}

	kdebugf("[PMM] User-reserved regions: %i\n", s_user_area.size());
	for(auto& reg : s_user_area) {
		kdebugf("  - PRegion(%x): start %x, size %i\n", reg, reg->addr(), reg->size());
	}
#endif
}


gen::SharedPtr<PageToken> _allocate_page_internal(gen::List<PRegion*>& area) {
	for(auto& range : area) {
		void* allocation = range->alloc_page();
		if(allocation) {
			auto ptr = gen::SharedPtr(new PageToken(allocation));
			Process::current()->make_page_owned(ptr);
			return ptr;
		}
	}
	return gen::SharedPtr<PageToken>{nullptr};
}

gen::SharedPtr<PageToken> PMM::allocate_page_user() {
	ASSERT_IRQ_DISABLED();

	auto token = _allocate_page_internal(s_user_area);
	if(!token) {
		kerrorf("[PMM] Could not find suitable region for allocating user page!");
		kpanic();
	}

	return token;
}

gen::SharedPtr<PageToken> PMM::allocate_page_kernel() {
	ASSERT_IRQ_DISABLED();

	auto token = _allocate_page_internal(s_kernel_area);
	if(!token) {
		kerrorf("[PMM] Could not find suitable region for allocating kernel page!");
		kpanic();
	}

	return token;
}

void PMM::free_page_from_token(PageToken* token) {
	ASSERT_IRQ_DISABLED();

	for(auto& range : s_user_area) {
		if(range->has_address(token->address())) {
			range->free_page(token->address());
#ifdef PMM_LOG_TOKEN_FREES
			kdebugf("[PMM] Freed %x from token! [user]\n", token->address());
#endif
			return;
		}
	}

	for(auto& range : s_kernel_area) {
		if(range->has_address(token->address())) {
			range->free_page(token->address());
#ifdef PMM_LOG_TOKEN_FREES
			kdebugf("[PMM] Freed %x from token! [kernel]\n", token->address());
#endif
			return;
		}
	}

	kpanic();
}
