%macro SAVE_REGS_CALLER 0
    push rax
    push rdi
    push rsi
    push rdx
    push rcx
    push r8
    push r9
    push r10
    push r11
%endmacro

%macro RESTORE_REGS_CALLER 0
    pop r11
    pop r10
    pop r9
    pop r8
    pop rcx
    pop rdx
    pop rsi
    pop rdi
    pop rax
%endmacro

%macro SAVE_REGS_CALLEE 0
    push rbp
    push rbx
    push r12
    push r13
    push r14
    push r15
%endmacro

%macro RESTORE_REGS_CALLEE 0
    pop r15
    pop r14
    pop r13
    pop r12
    pop rbx
    pop rbp
%endmacro

%macro SAVE_REGS_ALL 0
    push rax
    push rbx
    push rcx
    push rdx
    push rsi
    push rdi
    push rbp
    push r8
    push r9
    push r10
    push r11
    push r12
    push r13
    push r14
    push r15
%endmacro

%macro RESTORE_REGS_ALL 0
    pop r15
    pop r14
    pop r13
    pop r12
    pop r11
    pop r10
    pop r9
    pop r8
    pop rbp
    pop rdi
    pop rsi
    pop rdx
    pop rcx
    pop rbx
    pop rax
%endmacro

%macro SWAPGS_WHEN_NECESSARY 0
    cmp qword [rsp+8], 8
    je %%__skip_swapgs
    swapgs
%%__skip_swapgs:
%endmacro

%macro EXCEPT_SWAPGS_WHEN_NECESSARY 0
    cmp qword [rsp+16], 8
    je %%__skip_swapgs
    swapgs
%%__skip_swapgs:
%endmacro
