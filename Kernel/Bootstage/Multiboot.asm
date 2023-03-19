; Multiboot2-specific defines
MB2_MAGIC       equ 0xE85250D6
MB2_ARCH_32BIT  equ 0

MB2_ARCH        equ MB2_ARCH_32BIT
MB2_LEN         equ (mb2_header_end - mb2_header_start)

struc mb2_tag_entry_address
    .type   resw 1
    .flags  resw 1
    .size   resw 1
    .addr   resd 1
endstruc

;  Multiboot header
section .multiboot
align 8
mb2_header_start:
	dd MB2_MAGIC
	dd MB2_ARCH
    dd MB2_LEN
    dd -(MB2_MAGIC + MB2_ARCH + MB2_LEN)

mb2_header_end:
