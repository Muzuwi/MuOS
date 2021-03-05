%include "Arch/i386/asm/SaveRegs.asm"

section .text

%macro define_entrypoint_for_irq 1
irq%{1:1}:
    SWAPGS_WHEN_NECESSARY
    ;  Ensure proper alignment of PtraceRegs
    push qword %{1:1}
    SAVE_REGS_ALL
    mov rdi, %{1:1}
    mov rsi, rsp
extern _kernel_irq_dispatch
    call _kernel_irq_dispatch
    RESTORE_REGS_ALL
    add rsp, 8
    SWAPGS_WHEN_NECESSARY
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

