#ifndef KERNEL_SYMBOLS_H
#define KERNEL_SYMBOLS_H
#include <stdint.h>
#include <stddef.h>

/*
 *  This file contains extern definitions for linkage symbols of the kernel executable
 */

//  Start of kernel virtual address space
extern uint64_t _ukernel_virtual_start;

//  Start of physical identity map
extern uint64_t _ukernel_identity_start;

//  Start of the kernel heap
extern uint64_t _ukernel_heap_start;

extern uint64_t _ukernel_elf_start;
extern uint64_t _ukernel_elf_end;
extern uint64_t _ukernel_virtual_offset;

extern uint64_t _ukernel_kmalloc_start;
extern uint64_t _ukernel_kmalloc_end;

extern uint64_t _ukernel_preloader_physical;
extern uint64_t _ukernel_physical_start;

extern uint64_t _ukernel_text_start;
extern uint64_t _ukernel_text_end;

extern uint64_t _ukernel_virt_kstack_start;
extern uint64_t _ukernel_virt_kstack_end;

extern uint64_t _ukernel_shared_start;
extern uint64_t _ukernel_shared_end;

extern uint64_t _userspace_stack_start;
extern uint64_t _userspace_stack_end;

extern uint64_t _userspace_heap_start;
extern uint64_t _userspace_heap_end;

#endif