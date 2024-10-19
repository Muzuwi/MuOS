#pragma once
#include <Core/Error/Error.hpp>
#include <LibGeneric/BitFlags.hpp>

/*  Kernel virtual memory configuration
 */

/* Architecture includes go here */
#ifdef ARCH_IS_x86_64
#	include <Arch/x86_64/VM.hpp>
#endif

#ifndef KERNEL_VM_IDENTITY_BASE
#	error "Missing architecture definition: KERNEL_VM_IDENTITY_BASE (Base address of the identity region in virtual memory)"
#endif
#ifndef KERNEL_VM_IDENTITY_LEN
#	error "Missing architecture definition: KERNEL_VM_IDENTITY_LEN (Length of the identity region in virtual memory)"
#endif

/* Custom helpers go here */
#ifndef idmap
/* idmap - turn a given physical address to a pointer in the physical identity memory map space */
#	define idmap(physptr)                                                                                    \
		((physptr < KERNEL_VM_IDENTITY_BASE)                                                                  \
		         ? reinterpret_cast<uint8_t*>(KERNEL_VM_IDENTITY_BASE) + reinterpret_cast<uintptr_t>(physptr) \
		         : physptr)
#endif

#ifndef idunmap
/* idunmap - turn a given identity map pointer back to a physical address */
#	define idunmap(idptr)                                                                                  \
		((idptr >= KERNEL_VM_IDENTITY_BASE)                                                                 \
		         ? reinterpret_cast<uint8_t*>(idptr) - reinterpret_cast<uintptr_t>(KERNEL_VM_IDENTITY_BASE) \
		         : reinterpret_cast<uint8_t*>(idptr))
#endif

namespace arch {
	enum class PageFlags {
		///  Page is readable
		Read = 1U << 1U,
		///  Page is writable
		Write = 1U << 2U,
		///  Page is executable
		Execute = 1U << 3U,
		///  Page should be accessible in whatever the architecture considers user mode
		User = 1U << 4U,
		///  Page should be considered a large page
		///  The architecture defines what it considers a large page, and further
		///  alignment constraints will be placed on the physical address used for
		///  mappings. Large pages may not be supported everywhere.
		Large = 1U << 5U,
		///  Page should be considered a huge page
		///  The architecture defines what it considers a huge page, and further
		///  alignment constraints will be placed on the physical address used for
		///  mappings. Huge pages may not be supported everywhere.
		Huge = 1U << 6U
	};
	DEFINE_ENUM_BITFLAG_OPS(PageFlags);

	using PagingHandle = void*;

	///  Allocate the top-level paging structure
	///  This allocates a platform-specific paging structure and returns a pointer to it.
	///	 This pointer will usually point to physical memory and must not be used directly.
	core::Result<PagingHandle> addralloc();
	///  RECURSIVELY free the given top-level paging structure
	///  This will free pages allocated on ALL levels of the structure
	core::Error addrfree(PagingHandle);
	///  Clone a given paging structure
	///  This allocates a new top-level paging structure and clones all existing mappings
	///  from the source structure. Further modifications to the paging structure will
	///  only affect the clone, not the original mappings (with exception to the kernel
	///  ranges which must be shared across all paging structures).
	core::Result<PagingHandle> addrclone(PagingHandle root);

	///  Create a mapping between virt <-> phys in the given paging structure
	core::Error addrmap(PagingHandle, void* pptr, void* vptr, PageFlags flags);
	///  Unmap a given virtual address
	core::Error addrunmap(PagingHandle, void* vptr);
}