#pragma once

///  This file contains definitions for available linker symbols on x86_64

extern unsigned char _ukernel_virtual_start[];
extern unsigned char _ukernel_identity_start[];
extern unsigned char _ukernel_heap_start[];
extern unsigned char _ukernel_heap_end[];
extern unsigned char _ukernel_elf_start[];
extern unsigned char _ukernel_elf_end[];
extern unsigned char _ukernel_virtual_offset[];
extern unsigned char _ukernel_kmalloc_start[];
extern unsigned char _ukernel_kmalloc_end[];
extern unsigned char _ukernel_preloader_physical[];
extern unsigned char _ukernel_physical_start[];
extern unsigned char _ukernel_text_start[];
extern unsigned char _ukernel_text_end[];
extern unsigned char _ukernel_virt_kstack_start[];
extern unsigned char _ukernel_virt_kstack_end[];
extern unsigned char _ukernel_shared_start[];
extern unsigned char _ukernel_shared_end[];
extern unsigned char _userspace_stack_start[];
extern unsigned char _userspace_stack_end[];
extern unsigned char _userspace_heap_start[];
extern unsigned char _userspace_heap_end[];
