.section .entrypoint, "ax"

.globl boot0_entrypoint
boot0_entrypoint:
    /* Zero-initialize BSS section */
    la t0, __SECTION_BSS_START
    la t1, __SECTION_BSS_END
.bss_zero_loop:
    bge t0, t1, .bss_zero_done
    sw zero, 0(t0)
    addi t0, t0, 4
    j .bss_zero_loop
.bss_zero_done:
    /* Configure an early trap handler */
    la t0, boot0_exception_handler
    csrw stvec, t0
    /* Configure stack for the entrypoint */
    la t0, _boot0_stack
    addi sp, t0, 0
    /* Preserve boot args from the previous bootstage */
    addi sp, sp, -2*8
    sd a0, 8(sp)
    sd a1, 16(sp)

    /* Call static initializers before running C++ code */
    call _init

    /* Restore boot args */
    ld a1, 16(sp)
    ld a0, 8(sp)
    addi sp, sp, 2*8
    /* Jump to the C++ entrypoint */
    jal boot0_main
.loop:
    wfi
    j .loop

.section .text, "ax"

.align 4

.macro PUSH_ALL_REGISTERS
    addi sp, sp, -32*8
    sd x1, 1*8(sp)
    sd x2, 2*8(sp)
    sd x3, 3*8(sp)
    sd x4, 4*8(sp)
    sd x5, 5*8(sp)
    sd x6, 6*8(sp)
    sd x7, 7*8(sp)
    sd x8, 8*8(sp)
    sd x9, 9*8(sp)
    sd x10, 10*8(sp)
    sd x11, 11*8(sp)
    sd x12, 12*8(sp)
    sd x13, 13*8(sp)
    sd x14, 14*8(sp)
    sd x15, 15*8(sp)
    sd x16, 16*8(sp)
    sd x17, 17*8(sp)
    sd x18, 18*8(sp)
    sd x19, 19*8(sp)
    sd x20, 20*8(sp)
    sd x21, 21*8(sp)
    sd x22, 22*8(sp)
    sd x23, 23*8(sp)
    sd x24, 24*8(sp)
    sd x25, 25*8(sp)
    sd x26, 26*8(sp)
    sd x27, 27*8(sp)
    sd x28, 28*8(sp)
    sd x29, 29*8(sp)
    sd x30, 30*8(sp)
    sd x31, 31*8(sp)
.endm

.macro POP_ALL_REGISTERS
    ld x31, 31*8(sp)
    ld x30, 30*8(sp)
    ld x29, 29*8(sp)
    ld x28, 28*8(sp)
    ld x27, 27*8(sp)
    ld x26, 26*8(sp)
    ld x25, 25*8(sp)
    ld x24, 24*8(sp)
    ld x23, 23*8(sp)
    ld x22, 22*8(sp)
    ld x21, 21*8(sp)
    ld x20, 20*8(sp)
    ld x19, 19*8(sp)
    ld x18, 18*8(sp)
    ld x17, 17*8(sp)
    ld x16, 16*8(sp)
    ld x15, 15*8(sp)
    ld x14, 14*8(sp)
    ld x13, 13*8(sp)
    ld x12, 12*8(sp)
    ld x11, 11*8(sp)
    ld x10, 10*8(sp)
    ld x9, 9*8(sp)
    ld x8, 8*8(sp)
    ld x7, 7*8(sp)
    ld x6, 6*8(sp)
    ld x5, 5*8(sp)
    ld x4, 4*8(sp)
    ld x3, 3*8(sp)
    ld x2, 2*8(sp)
    ld x1, 1*8(sp)
    addi sp, sp, 32*8
.endm


.globl boot0_exception_handler
boot0_exception_handler:
.cfi_startproc
    PUSH_ALL_REGISTERS

    csrr a0, scause
    csrr a1, sepc
    csrr a2, stval

.globl boot0_handle_exception
    jal boot0_handle_exception

    POP_ALL_REGISTERS
    sret

.trap_dead:
    j .trap_dead
.cfi_endproc

.section .data, "aw"
/* just define the section for the linker */

.section .bss, "aw"
_boot0_stack_bottom:
    .skip 4096
.globl _boot0_stack
_boot0_stack:
