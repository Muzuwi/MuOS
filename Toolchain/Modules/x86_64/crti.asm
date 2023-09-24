; Sources for the kernel's crti.o
; The default crti.o built for the cross-compiled toolchain
; does not preserve the stack pointer, this explicitly adds
; the preserving prologue to _init/_fini.
; The only real reason we need this is that the kernel uses
; some static variables which need to be initialized even before
; jumping to the kernel entrypoint..
; Needless to say, that is very very terrible and should be removed
; in favor of explicit initialization. This is a bandaid for this
; terribleness.

bits 64

section .init
global _init:function
_init:
    push rbp
    mov rbp, rsp

section .fini
global _fini:function
_fini:
    push rbp
    mov rbp, rsp
