section .data
retval:
    dd 0

section .text
global _start
_start:
    extern _init
    call _init

    extern main
    call main
    mov [retval], eax

    extern _fini
    call _fini

    mov eax, 1
    mov ebx, [retval]
    int 0x80
    hlt