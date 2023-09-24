; Sources for the kernel's crtn.o
; This adds the RSP-preserving epilogue to .init/.fini. See `crti.asm` for
; an explanation of why this is currently required.

bits 64

section .init
    pop rbp
    ret

section .fini
    pop rbp
    ret
