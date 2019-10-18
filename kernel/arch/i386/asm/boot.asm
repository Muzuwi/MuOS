; Header things
ALIGNONPAGE equ 1 << 0
MEMINFO equ 1 << 1
MAGIC equ 0x1BADB002

FLAG equ ALIGNONPAGE | MEMINFO
CHECKSUM equ -(MAGIC + FLAG)

; Initialize multiboot for Grub support
section .multiboot
; Align on 4 byte words
align 4
	dd MAGIC
	dd FLAG
	dd CHECKSUM

; Initialize .bss for stack
section .bss
; Align on 16 byte words, to conform to standards
align 16
stack_bottom:
; Allocate 16 KiB
resb 16384
stack_top:

; The fun things
section .text
global _start
_start:
	;  Set stack
	mov esp, stack_top

	;   Call fancy init
	extern _init
	call _init

	; 	Call the kernel
	extern kernel_entrypoint
	call kernel_entrypoint

	extern _fini
	call _fini

	loopLabel:
		cli
		hlt
		jmp loopLabel

.end: