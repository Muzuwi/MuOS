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
	;  The boostrap takes care of jumping to _ukernel_higher_entrypoint
    extern _paging_bootstrap
	mov eax, _paging_bootstrap
	sub eax, _ukernel_virtual_offset
	call eax

    ;  If we reach this, something has gone terribly wrong
dead:
    cli
    hlt
    jmp dead

;       Calling constructors and calling the actual kernel entrypoint
global _ukernel_higher_entrypoint
_ukernel_higher_entrypoint:
    ;  Reset stack address
    mov eax, stack_top
    mov esp, eax
    mov ebp, 0  ; Null function frame for stack tracing

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
