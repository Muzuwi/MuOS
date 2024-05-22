
.section .bootstrap, "ax"

.globl _entrypoint_riscv64
_entrypoint_riscv64:
    # hifive unleashed UART
    li t1, 0x10010000

    # set txen=1
    li t0, 0x1
    sw t0, 8(t1)

    li t0, 'H'
    sw t0, 0(t1)
    li t0, 'e'
    sw t0, 0(t1)
    li t0, 'l'
    sw t0, 0(t1)
    li t0, 'l'
    sw t0, 0(t1)
    li t0, 'o'
    sw t0, 0(t1)
    li t0, '!'
    sw t0, 0(t1)
.loop:
    wfi
    j .loop
