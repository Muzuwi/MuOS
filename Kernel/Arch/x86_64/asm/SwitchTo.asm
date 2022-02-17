%include "Arch/x86_64/asm/SaveRegs.asm"

section .text

;  rdi - prev task, rsi - next task
global _switch_to_asm
_switch_to_asm:
    SAVE_REGS_CALLEE
    pushfq

    ;  Save current frame
    mov [rdi + 0x0], rsp
    ;  Restore frame of next task
    mov rsp, [rsi + 0x0]

    ;  Restore cr3 of next task
    mov rax, [rsi + 0x18]
    mov cr3, rax

    popfq
    RESTORE_REGS_CALLEE
    ;  Finalize task switch
extern _ZN6Thread15finalize_switchEPS_S0_
    jmp _ZN6Thread15finalize_switchEPS_S0_

;   All new tasks start here
;   By modifying the PtraceRegs on the kernel stack, we can control
;   register state when entering the task
global _task_enter_bootstrap
_task_enter_bootstrap:
    RESTORE_REGS_ALL
    ;  Skip over PtraceRegs.origin
    add rsp, 8

    ;  When switching kernel->user, do swapgs so that gsbase represents userland gsbase.
    ;  Otherwise, leave gsbase pointing to the current CTB for kernel threads
    cmp qword [rsp+8], 8
    je ._skip_swapgs
    swapgs
    ._skip_swapgs:

    ;  Enter task for the first time
    iretq


;   Does a mode switch to userland for the first time. Pretty
;   much the same thing as _task_enter_bootstrap, except at this
;   point it would be dangerous to modify the kernel stack as we're
;   currently running on it!
;   Used by code running in kernel context to initialize execution of a userland thread
;   rdi - pointer to a PtraceRegs structure, representing
;         the initial register state of the program
global _bootstrap_user
_bootstrap_user:
    ;  Use the structure as a stack
    mov rsp, rdi
    ;  Restore all GPRs
    RESTORE_REGS_ALL

    ;  Skip over the origin field
    add rsp, 8

    ;  Unconditionally swapgs
    ;  This function is only to be used for KERNEL->USER switches
    swapgs

    ;  Enter userland
    iretq
