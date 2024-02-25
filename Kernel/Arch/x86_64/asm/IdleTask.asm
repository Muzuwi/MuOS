bits 64
section .text

global platform_idle:
platform_idle:
.loop:
    hlt
    jmp .loop
