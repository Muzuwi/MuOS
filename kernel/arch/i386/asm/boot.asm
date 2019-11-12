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
	dd 0
	dd 0
	dd 0
	dd 0
	dd 0

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
	;  We have to subtract the virtual offset from the address,
	;  to get the physical one, because the linker puts the 
	;  object at higher half 
	extern _ukernel_virtual_offset
	mov eax, stack_top
	sub eax, _ukernel_virtual_offset
	mov esp, eax

	;  Call stage0
	;  This basically sets up higher half mappings
	;  but in c, because ASM does not bring joy to me
	;  The kernel is mapped to 0xd0000000+1MiB
	extern _stage0_entrypoint
	mov eax, _stage0_entrypoint
	sub eax, _ukernel_virtual_offset
	call eax

	;  Correct stack address
	mov eax, esp
	add eax, _ukernel_virtual_offset
	mov esp, eax

	;  Do a far jump into higher half
	mov eax, higher
	push eax
	ret
higher:

	;   Call fancy init
	extern _init
	call _init

	;  Pass the address of the multiboot info struct to the kernel
	push ebx

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