ENTRY(_entrypoint_x32)

/*
    Kernel virtual memory layout
    need 4000_00000000 for identity map
    for now, only non-PIE executables

  00000000_00000000........00007fff_ffffffff - userspace
   |00001000_00000000........00001fff_ffffffff - executable
   |00002000_00000000........00002fff_ffffffff - <hole>
   |00003000_00000000........00006fff_ffffffff - heap (4TB)
   |00007000_00000000........00007ffe_ffffffff - <hole>
   |00007fff_00000000........00007fff_ffffffff - stack
   |    Upper word is randomized - the stack for each process
   |    is somewhere at 00007ffff_xxxx0000 - 00007fff_xxxxffff
   \    With ASLR disabled, the stack is locked at 00007ffff_ffff0000 - 00007ffff_ffffffff
  ffff8000_00000000........ffffffff_ffffffff - kernel
   |ffff8000_00000000........ffffbfff_ffffffff - identity map of all physical memory (64TB)
   |ffffc000_00000000........ffffcfff_ffffffff - kernel heap (1TB)
   |ffffd000_00000000........ffffd000_0fffffff - kernel ELF (256MB)
   |ffffe000_00000000........ffffe0ff_ff800000 - kernel task stacks (ASLR) - 0x3fffe00 possible kstack locations
   | Rest is unused for now
   \
*/

_userspace_virtual_start = 0x0;
_userspace_virtual_end = 0x00007fffffffffff;
_userspace_stack_start = 0x00007fff00000000;
_userspace_stack_end = 0x00007fffffffffff;
_userspace_heap_start = 0x0000300000000000;
_userspace_heap_end = 0x00006fffffffffff;

_ukernel_virtual_start = 0xffff800000000000;
_ukernel_identity_start = 0xffff800000000000;
_ukernel_heap_start = 0xffffc00000000000;
_ukernel_heap_end = 0xffffd00000000000;
_ukernel_elf_start = 0xffffd00000000000;
_ukernel_virt_heap_end = 0xffffcfffffffffff;
_ukernel_virt_elf_end = 0xffffd00010000000;
_ukernel_virt_kstack_start = 0xffffe00000000000;
_ukernel_virt_kstack_end = 0xffffe0ffff800000;

_ukernel_shared_start = _ukernel_virtual_start;
_ukernel_shared_end = 0xffffdfffffffffff;

/*  Physical address of the preloader */
_ukernel_preloader_physical = 0x100000;

/*  Physical start address of the kernel */
_ukernel_physical_start = 0x110000;

/*  Offset from virtual to physical ELF address */
_ukernel_virtual_offset = _ukernel_elf_start - _ukernel_physical_start;

SECTIONS
{
    . = _ukernel_preloader_physical;

    /*  Load bootstrap into lowmem  */
    .bootstrap ALIGN(4K) : AT(_ukernel_preloader_physical) {
        *(.multiboot)
		*(.bootstrap32)
    }

    . = _ukernel_elf_start;

    /*  64-bit kernel text section  */
	.text ALIGN(4K) : AT(ADDR(.text) - _ukernel_virtual_offset) {
	    _ukernel_text_start = .;
	    *(.entrypoint)
		*(.text)
		*(.text.*)
		_ukernel_text_end = .;
	}

   	/*  RO Data  */
   	.rodata ALIGN(4K) : AT(ADDR(.rodata) - _ukernel_virtual_offset) {
   	    _ukernel_ro_start = .;
   		*(.rodata)
   		_ukernel_ro_end = .;
   	}

   	/*  R/W Data  */
   	.data ALIGN(4K) : AT(ADDR(.data) - _ukernel_virtual_offset) {
   		*(.data)
   	}

   	/*  Uninitialized Read-write data + stack  */
   	.bss ALIGN(4K) : AT(ADDR(.bss) - _ukernel_virtual_offset) {
   		*(.bss)
   	}

    /*  Make sure the kernel has a page-aligned size  */
    _ukernel_elf_end = ALIGN(0x1000);

   	_ukernel_elf_size = _ukernel_elf_end - _ukernel_elf_start;

    /*  +1 PDPT, +1 PD, +1 PT, +1 PT for each 2MiB of _ukernel_elf_size  */
    __extra_pages = _ukernel_elf_size;
    __extra_pages /= 2*1024*1024;

    _ukernel_elf_pages_needed = 3 + __extra_pages;
    /*  Used by the 32-bit entrypoint, to avoid having to calculate this manually */
    _ukernel_elf_pages_needed_in_bytes = _ukernel_elf_pages_needed * 4096;
}