%include "Arch/i386/asm/SaveRegs.asm"

section .text

;  rdi - prev task, rsi - next task
global _switch_to_asm
_switch_to_asm:
    SAVE_REGS_CALLEE

    ;  Save current frame
    mov [rdi + 0x0], rsp
    ;  Restore frame of next task
    mov rsp, [rsi + 0x0]

    ;  Restore cr3 of next task
    mov rax, [rsi + 0x8]
    mov cr3, rax

    RESTORE_REGS_CALLEE
    ;  Finalize task switch
extern _ZN7Process15finalize_switchEPS_S0_
    jmp _ZN7Process15finalize_switchEPS_S0_

;   All new tasks start here
;   By modifying the PtraceRegs on the kernel stack, we can control
;   register state when entering the task
global _task_enter_bootstrap
_task_enter_bootstrap:
    RESTORE_REGS_ALL
    ;  Skip over PtraceRegs.origin
    add rsp, 8
    ;  Enter task for the first time
    iretq

