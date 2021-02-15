%include "Arch/i386/asm/SaveRegs.asm"

section .text

global _kernel_syscall_entry
_kernel_syscall_entry:
._stub:
    jmp ._stub
    iretq

%macro define_entrypoint_for_irq 1
;global irq%{1:1}
irq%{1:1}:
    SAVE_REGS_ALL

    mov rdi, %{1:1}
    mov rsi, rsp
extern _kernel_irq_dispatch
    call _kernel_irq_dispatch

    RESTORE_REGS_ALL
    iretq
%endmacro

%assign i 32
%rep 256-32
define_entrypoint_for_irq %[i]
%assign i i+1
%endrep

section .data
align 8
global irq_entrypoint_table
irq_entrypoint_table:
%assign i 32
%rep 256-32
    dq irq%[i]
%assign i i+1
%endrep

