#pragma once
#include <Arch/x86_64/LinkscriptSyms.hpp>

/* Base address of the identity region in virtual memory */
#define KERNEL_VM_IDENTITY_BASE (reinterpret_cast<void*>(&_ukernel_identity_start))
/* Length of the identity region in virtual memory */
#define KERNEL_VM_IDENTITY_LEN \
	(reinterpret_cast<uintptr_t>(&_ukernel_heap_start) - reinterpret_cast<void*>(&_ukernel_identity_start))
