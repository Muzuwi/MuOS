ENTRY(boot0_entrypoint)

/* Physical address of boot0 binary */
__BOOT0_PHYS_BASE = 0x80400000;
/* Max length of the boot0 binary */
__BOOT0_MAXLEN    = 0x00010000;

MEMORY
{
    BOOT0    (rwx)  : ORIGIN = __BOOT0_PHYS_BASE, LENGTH = __BOOT0_MAXLEN
}

SECTIONS
{
    __SECTION_TEXT_START = .;
	.text : ALIGN(4K) {
	    KEEP(*(.entrypoint .entrypoint.*))
		KEEP(*(.text .text.*))
	} >BOOT0
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
   	}>BOOT0
    .init_array : ALIGN(8) {
        KEEP (*(SORT(EXCLUDE_FILE(crti.o crtn.o) .init_array.*)))
        KEEP (*(EXCLUDE_FILE(crti.o crtn.o) .init_array))
    }>BOOT0
    .fini_array : ALIGN(8) {
        KEEP (*(SORT(EXCLUDE_FILE(crti.o crtn.o) .fini_array.*)))
        KEEP (*(EXCLUDE_FILE(crti.o crtn.o) .fini_array))
    }>BOOT0
    __SECTION_RODATA_END = .;

    __SECTION_DATA_START = .;
   	.data : ALIGN(4K) {
   		KEEP(*(.data .data.*))
        KEEP(*(.sdata .sdata.*))
   	}>BOOT0
    __SECTION_DATA_END = .;

    __SECTION_BSS_START = .;
   	.bss : ALIGN(4K) {
   		KEEP(*(.bss .bss.*))
   		KEEP(*(.sbss .sbss.*))
   	}>BOOT0
    __SECTION_BSS_END = .;

    . = ALIGN(4K);
    __BOOT0_PHYS_END = ORIGIN(BOOT0) + LENGTH(BOOT0);
}