#pragma once
#include <Arch/VM.hpp>
#include <Core/Error/Error.hpp>
#include <SystemTypes.hpp>

/*	core::mem - *kernel* VM management subsystem
 *
 *	All tasks running in the kernel share the same upper portion
 *	of the address space, which is reserved exclusively for the
 *	kernel itself. This subsystem manages modifications to the
 *	kernel mappings, so that they can be performed safely between
 *	multiple nodes running in parallel (SMP).
 */
namespace core::mem {
	/*	Allocate memory within the vmalloc area.
	 *
	 *	This function allocates a chunk of virtual memory that is immediately
	 *  available for use by the kernel. `vmalloc` can only allocate chunks
	 * 	that are a multiple of the current page-size. Requests for less than
	 * 	that will be forcibly aligned to the next multiple of the page size,
	 * 	making `vmalloc` unsuitable for usage as a generic heap (but it can
	 * 	itself be used for allocating the backing store for a heap).
	 */
	void* vmalloc(size_t);

	/*	Free memory previously allocated with vmalloc.
	 *
	 * 	Frees a chunk of virtual memory previously allocated using `vmalloc`.
	 * 	The underlying physical pages used by the allocation are freed, and
	 * 	the virtual address will be reusable by future allocations.
	 *
	 *	WARNING: This function is currently unimplemented!
	 */
	void vfree(void*);

	/*	Get the kernel root paging handle.
	 *
	 *	All modifications to kernel address space must go through the root
	 *	paging handle or addrclone'd versions thereof.
	 */
	arch::PagingHandle get_vmroot();
}