%include "Arch/x86_64/asm/SaveRegs.asm"

section .text

%macro def_entry_exception 1
_exception_entry_%{1:1}:
    ;  Exception does not provide error code, pad with 0 for proper PtraceRegs struct alignment
    push dword 0
    EXCEPT_SWAPGS_WHEN_NECESSARY
    SAVE_REGS_ALL
    mov rdi, %{1:1}
    mov rsi, rsp
extern _kernel_exception_entrypoint
    call _kernel_exception_entrypoint
    RESTORE_REGS_ALL
    EXCEPT_SWAPGS_WHEN_NECESSARY
    ;  Clear padding for PtraceRegs
    add rsp, 8
    iretq
%endmacro

%macro def_entry_exception_errorcode 1
_exception_entry_%{1:1}:
    EXCEPT_SWAPGS_WHEN_NECESSARY
    SAVE_REGS_ALL
    mov rdi, %{1:1}
    mov rsi, rsp
extern _kernel_exception_entrypoint
    call _kernel_exception_entrypoint
    RESTORE_REGS_ALL
    EXCEPT_SWAPGS_WHEN_NECESSARY
    add rsp, 8
    iretq
%endmacro

def_entry_exception 0
def_entry_exception 1
def_entry_exception 2
def_entry_exception 3
def_entry_exception 4
def_entry_exception 5
def_entry_exception 6
def_entry_exception 7
def_entry_exception_errorcode 8
def_entry_exception 9
def_entry_exception_errorcode 10
def_entry_exception_errorcode 11
def_entry_exception_errorcode 12
def_entry_exception_errorcode 13
def_entry_exception_errorcode 14
def_entry_exception 15
def_entry_exception 16
def_entry_exception_errorcode 17
def_entry_exception 18
def_entry_exception 19
def_entry_exception 20
def_entry_exception 21
def_entry_exception 22
def_entry_exception 23
def_entry_exception 24
def_entry_exception 25
def_entry_exception 26
def_entry_exception 27
def_entry_exception 28
def_entry_exception 29
def_entry_exception_errorcode 30
def_entry_exception 31

section .data
align 8
global exception_entrypoint_table
exception_entrypoint_table:
%assign i 0
%rep 32
    dq _exception_entry_%[i]
%assign i i+1
%endrep