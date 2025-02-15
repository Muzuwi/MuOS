ENTRY(_riscv64_virtual_trampoline)

/* Virtual base address of the kernel binary */
__KERNEL_VM_BASE   = 0xFFFFFFC000000000;
/* Max length of the kernel binary */
__KERNEL_VM_MAXLEN = 0x0000001000000000;

MEMORY
{
    KERNELVM (rwx)  : ORIGIN = __KERNEL_VM_BASE, LENGTH = __KERNEL_VM_MAXLEN
}

SECTIONS
{
    __SECTION_TEXT_START = .;
	.text : ALIGN(4K) {
        KEEP(*(.kernel_header))
	    KEEP(*(.entrypoint .entrypoint.*))
		KEEP(*(.text .text.*))
	} >KERNELVM
    __SECTION_TEXT_END = .;

    __SECTION_RODATA_START = .;
   	.rodata : ALIGN(4K) {
        KEEP(*(.rodata .rodata.*))
   		KEEP(*(.srodata .srodata.*))
        /* Explicitly put these somewhere so they do not create a separate
         * section for no reason due to being orphaned. We do not use exceptions,
         * they will never actually be used. These cannot be DISCARD'ed as they
         * are referenced in crtbegin.o of the cross-compiler. */
        *(.eh_frame)
        *(.tm_clone_table)
   	} >KERNELVM
    .init_array : ALIGN(8) {
        */crti.o(.init_array)
        KEEP (*(SORT(EXCLUDE_FILE(*/crti.o */crtn.o) .init_array.*)))
        KEEP (*(EXCLUDE_FILE(*/crti.o */crtn.o) .init_array))
        */crtn.o(.init_array)
    } >KERNELVM
    .fini_array : ALIGN(8) {
        */crti.o(.fini_array)
        KEEP (*(SORT(EXCLUDE_FILE(*/crti.o */crtn.o) .fini_array.*)))
        KEEP (*(EXCLUDE_FILE(*/crti.o */crtn.o) .fini_array))
        */crtn.o(.fini_array)
    } >KERNELVM
    __SECTION_RODATA_END = .;

    __SECTION_DATA_START = .;
   	.data : ALIGN(4K) {
   		KEEP(*(.data .data.*))
        KEEP(*(.sdata .sdata.*))
   	} >KERNELVM
    __SECTION_DATA_END = .;

    __SECTION_BSS_START = .;
   	.bss : ALIGN(4K) {
   		KEEP(*(.bss .bss.*))
   		KEEP(*(.sbss .sbss.*))
   	} >KERNELVM
    __SECTION_BSS_END = .;

    /* Make sure the kernel has a page-aligned end address */
    . = ALIGN(4K);
    __KERNEL_VM_END = .;
   	__KERNEL_VM_SIZE = __KERNEL_VM_END - __KERNEL_VM_BASE;
}