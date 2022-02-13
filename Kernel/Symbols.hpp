#ifndef KERNEL_SYMBOLS_H
#define KERNEL_SYMBOLS_H
#include <stdint.h>
#include <stddef.h>

/*
 *  This file contains extern definitions for linkage symbols of the kernel executable
 */

//  Start of kernel virtual address space
extern void* _ukernel_virtual_start;

//  Start of physical identity map
extern void* _ukernel_identity_start;

extern void* _ukernel_heap_start;
extern void* _ukernel_heap_end;

extern void* _ukernel_elf_start;
extern void* _ukernel_elf_end;
extern void* _ukernel_virtual_offset;

extern void* _ukernel_kmalloc_start;
extern void* _ukernel_kmalloc_end;

extern void* _ukernel_preloader_physical;
extern void* _ukernel_physical_start;

extern void* _ukernel_text_start;
extern void* _ukernel_text_end;

extern void* _ukernel_virt_kstack_start;
extern void* _ukernel_virt_kstack_end;

extern void* _ukernel_shared_start;
extern void* _ukernel_shared_end;

extern void* _userspace_stack_start;
extern void* _userspace_stack_end;

extern void* _userspace_heap_start;
extern void* _userspace_heap_end;

#endif