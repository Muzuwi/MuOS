
.section .bootstrap, "ax"

.globl _entrypoint_riscv64
_entrypoint_riscv64:
    wfi
    j _entrypoint_riscv64
