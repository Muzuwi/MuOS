#ifndef KERNEL_SYMBOLS_H
#define KERNEL_SYMBOLS_H

/*
 *  This file contains extern definitions for linkage symbols of the kernel executable
 */

//  Start of kernel virtual address space
extern uint32_t _ukernel_virtual_offset;

// 	Helper macros to convert between physical->virtual and virtual->physical addresses
#define TO_VIRT(a) ((uintptr_t)a + (uintptr_t)&_ukernel_virtual_offset)
#define TO_PHYS(a) ((uintptr_t*)((uintptr_t)a - (uintptr_t)&_ukernel_virtual_offset))

//  Stack area for kernel tasks
//  TODO:  Will be removed
extern uint32_t _ukernel_tasks_stack;

//  Interrupt stack of the kernel
extern uint32_t _ukernel_interrupt_stack;

//  Start and end of the kernel binary
extern uint32_t _ukernel_start, _ukernel_end;

//  Page table buffer for all possible kernel page tables
extern uint32_t _ukernel_pages_start;

//  Start and end of read-only sections in the kernel binary
extern uint32_t _ukernel_RO_begin, _ukernel_RO_end;

//  Kernel higher half entrypoint
extern uint32_t _ukernel_higher_entrypoint;

//  KMalloc region start
extern uint32_t _ukernel_kmalloc_start;

//  KMalloc region end
extern uint32_t _ukernel_kmalloc_end;

#endif