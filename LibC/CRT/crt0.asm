section .text
global _start
_start:
    ;  Call constructors
    extern _init
    call _init

    ;  Call main
    extern main
    call main

    ;  Save return value from main
    push rax

    ;  Call destructors
    extern _fini
    call _fini

    pop rax

    ;  Exit process
    hlt
    ud2