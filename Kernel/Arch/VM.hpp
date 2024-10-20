#pragma once

/*  Kernel virtual memory configuration
 */

/* Base address of the identity region in virtual memory */
#undef KERNEL_VM_IDENTITY_BASE
/* Length of the identity region in virtual memory */
#undef KERNEL_VM_IDENTITY_LEN

/* Architecture includes go here */
#ifdef ARCH_IS_x86_64
#	include <Arch/x86_64/VM.hpp>
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
