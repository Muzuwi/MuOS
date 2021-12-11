section .text
bits 64

global loadGDT
loadGDT:
    push rbp
    mov rbp, rsp

	mov rax, rdi
	lgdt [rax]

    mov rax, 8
    ;    mov [rbp+8], rax
    push rax
    mov rax, .reloadSelectors
    ;    mov [rbp], rax
    push rax

    retfq

;	mov rax, ._tmpbuf
;	jmp far [rax]
;align 8
;._tmpbuf:
;	dq .reloadSelectors  ;  Address
;	dw 0x08              ;  Selector
.reloadSelectors:
	mov ax, 0x10
	mov ds, ax
	mov es, ax
	mov fs, ax
	mov gs, ax
	mov ss, ax

    mov rsp, rbp
    pop rbp
	ret
