%include "Arch/i386/asm/SaveRegs.asm"

section .text

;  Global syscall entrypoint
global _ukernel_syscall_entry:
_ukernel_syscall_entry:
    ;  Switch to kernel GS (CTB)
    swapgs

    ;  Temporarily save user stack to CTB scratch space
    mov gs:0x18, rsp
    mov rsp, gs:0x8        ;  rsp - task pointer
    mov rsp, [rsp + 0x8]  ;  now on kernel stack
    ;  Save user stack from CTB
    push qword [gs:0x18]

    ;  Provide PtraceRegs.origin
    push qword 0
    push qword 5
    push qword 5
    push qword 5
    push qword 5
    push qword 5

    ;  Save user registers
    SAVE_REGS_ALL

    mov rdi, rsp

    sti
extern _ZN7Syscall14syscall_handleEP10PtraceRegs
    call _ZN7Syscall14syscall_handleEP10PtraceRegs
    cli

    RESTORE_REGS_ALL

    ;  Skip PtraceRegs.origin
    add rsp, 8+5*8
    ;  Restore user stack
    mov rsp, [rsp]

    ;  Restore userland GS
    swapgs

    o64 sysret

