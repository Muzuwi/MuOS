section .text
%include "Arch/x86_64/asm/SaveRegs.asm"

bits 64

;  Clobbers rax, rcx, rdx, outputs in rax
%macro READMSR 1
    mov rcx, %1
    rdmsr
    shl rdx, 32
    or rax, rdx
%endmacro

;  input in rax, clobbers rcx, rdx
%macro WRITEMSR 1
    mov rcx, %1
    mov rdx, rax
    shr rdx, 32
    and rax, 0x00000000FFFFFFFF
    wrmsr
%endmacro


;  rdi - phys, low mem page for code/data
;  rsi - phys, low mem page for stack
;  rdx - real mode irq number to call
;  rcx - address of the V86Regs structure
global vm86_irq
vm86_irq:
    cli
    SAVE_REGS_ALL
    pushf
    push rcx

    push rdx
    push rcx

    ;  Copy over shellcode to the code page
    mov rax, rdi
    mov rbx, vm86_shellcode_start
    mov rdx, vm86_shellcode_end
.shell_copy:
    mov cl, byte [rbx]
    mov byte [rax], cl
    inc rax
    inc rbx
    cmp rbx, rdx
    jne .shell_copy

    pop rcx
    pop rdx
    push rdx
    push rcx

    ;  Copy over the values of the reg structure to the code page
    lea rax, [rdi + (vm86_real_regs-vm86_shellcode_start)]
    mov rbx, vm86_real_regs_end-vm86_real_regs
.copy_regs:
    mov dl, byte [rcx]
    mov byte [rax], dl
    inc rax
    inc rcx
    dec rbx
    test rbx, rbx
    jnz .copy_regs

    pop rcx
    pop rdx

    ;  Save full-width CR3
    mov rax, cr3
    lea rbx, [edi + (vm86_saved_cr3 - vm86_shellcode_start)]
    mov [rbx], rax
    ;  Save full-width GDTR
    lea rax, [edi + (vm86_saved_gdtr - vm86_shellcode_start)]
    sgdt [rax]
    ;  Save full-width IDTR
    lea rax, [edi + (vm86_saved_idtr - vm86_shellcode_start)]
    sidt [rax]
    ;  Save rsp
    lea rax, [edi + (vm86_saved_rsp - vm86_shellcode_start)]
    mov [rax], rsp

    push rcx
    push rdx

    ;  Save EFER
    READMSR 0xC0000080
    lea rbx, [edi + (vm86_saved_efer - vm86_shellcode_start)]
    mov [rbx], rax
    ;  Save the xBASE registers
    ;  TODO: might be unnecessary?
    READMSR 0xC0000100
    lea rbx, [edi + (vm86_saved_fsbase - vm86_shellcode_start)]
    mov [rbx], rax
    READMSR 0xC0000101
    lea rbx, [edi + (vm86_saved_gsbase - vm86_shellcode_start)]
    mov [rbx], rax
    READMSR 0xC0000102
    lea rbx, [edi + (vm86_saved_kgsbase - vm86_shellcode_start)]
    mov [rbx], rax

    pop rdx
    pop rcx

    ;  Patch compatibility mode GDTR offset
    mov eax, edi
    lea ebx, [edi + (vm86_compat_gdtr - vm86_shellcode_start) + 2]  ; address of the GDTR offset field
    add [ebx], edi
    ;  Patch long-mode bootstrap GDTR offset
    mov eax, edi
    lea ebx, [edi + (vm86_long_gdtr - vm86_shellcode_start) + 2]  ; address of the GDTR offset field
    add [ebx], edi
    ;  Change into compatibility (protected) mode
    lea rax, [edi + (vm86_compat_gdtr - vm86_shellcode_start)]
    lgdt [rax]

    mov rax, 8
    push rax
    mov rax, rdi
    push rax
    retfq

align 4096
global vm86_shellcode_start
vm86_shellcode_start:
%define OFFSETOF(a) (a - vm86_shellcode_start)
    bits 32
    mov eax, 0x10
    mov ds, eax
    mov ss, eax
    mov es, eax
    ;  On enter, edi contains the shellcode page, and esi contains the stack page

    ;  Disable paging
    mov eax, cr0
    and eax, ~((1 << 31))
    mov cr0, eax
    ;  Zero out CR3
    xor eax, eax
    mov cr3, eax

    ;  Patch GDTR offset
    mov eax, edi
    lea ebx, [edi + OFFSETOF(vm86_real_gdtr) + 2] ; address of the GDTR offset field
    add [ebx], edi

    ;  Change to real-mode GDT
    lea ebx, [edi + OFFSETOF(vm86_real_gdtr)] ;  address of the GDTR
    lgdt [ebx]

    ;  Set up a proper real-mode stack
    lea ecx, [esi + 0x1000]
    mov esp, ecx
    mov ebp, ecx

    ;  Patch the second byte of the INT instruction in 16-bit code
    lea ebx, [edi + OFFSETOF(vm86_softirq_byte)]
    mov byte [ebx], dl

    ;  Far jump to 16-bit protected mode
    lea eax, [edi + OFFSETOF(vm86_shellcode_16bit)]
    push dword 0x8
    push eax
    retf

align 8
bits 16
vm86_shellcode_16bit:
    ;  Set selectors for lowmem
    mov ax, 0x10
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov ss, ax

    ;  Restore original IVT
    lea eax, [edi + OFFSETOF(vm86_real_idtr)]
    lidt [eax]

    ;  Disable protected mode
    mov eax, cr0
    and eax, ~((1 << 0))
    mov cr0, eax

    ;  Far jump to real mode
    lea eax, [edi + OFFSETOF(vm86_shellcode_real)]
    push dword 0x0
    push eax
    retf
vm86_shellcode_real:
    ;  Make sure segments are zero
    xor ax, ax
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov ss, ax

    ;  Save our data page
    push di

    ;  Make saving register state after the IRQ easier by
    ;  preconstructing the address
    lea ax, [di + OFFSETOF(vm86_real_regs_end)]
    push ax

    ;  Set register state
    mov bp, sp
    lea sp, [di + OFFSETOF(vm86_real_regs)]
    pop ax
    pop bx
    pop cx
    pop dx
    pop si
    pop di
    pop es
    xchg ebp, esp

    ;  Execute the IRQ
    db 0xCD
vm86_softirq_byte:  ;  Dynamically patch the byte to the proper IRQ
    db 0x00

    ;  Restore the address of the regs struct and save the V86 registers
    pop bp
    xchg ebp, esp
    push es
    push di
    push si
    push dx
    push cx
    push bx
    push ax
    xchg ebp, esp

    ;  Restore the code/data page
    pop di

    ;  Reenable protection
    mov eax, cr0
    or eax, (1 << 0)
    mov cr0, eax

    ;  Restore 16-bit protected mode GDTR
    lea eax, [edi + OFFSETOF(vm86_real_gdtr)]
    lgdt [eax]
    lea eax, [edi + OFFSETOF(vm86_return_prot16)]
    ;  Far jump to 16-bit protected mode
    push dword 0x8
    push eax
    o32 retf
vm86_return_prot16:
    mov ax, 0x10
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov ss, ax
    ;  Restore compat GDTR
    lea eax, [edi + OFFSETOF(vm86_compat_gdtr)]
    lgdt [eax]

    lea eax, [edi + OFFSETOF(vm86_return_compat)]
    push dword 0x8
    push eax
    o32 retf
vm86_return_compat:
bits 32
    ;  FIXME: Narrowing CR3 to 32-bit, will cause a total failure if a processes' CR3 is in physical memory >4 GiB
    lea eax, [edi + OFFSETOF(vm86_saved_cr3)]
    mov eax, [eax]
    mov cr3, eax
    ;  Re-enable paging
    mov eax, cr0
    or eax, (1 << 31)
    mov cr0, eax
    ;  Switch to long-mode bootstrap GDTR
    lea eax, [edi + OFFSETOF(vm86_long_gdtr)]
    lgdt [eax]
    ;  Far jump back into long mode
    lea eax, [edi + OFFSETOF(vm86_return_long)]
    push 0x8
    push eax
    retf
vm86_return_long:
bits 64
    ;  Restore actual kernel GDTR
    lea rax, [edi + OFFSETOF(vm86_saved_gdtr)]
    lgdt [rax]
    lea rax, [edi + OFFSETOF(vm86_return_long_stage2)]
    push 0x8
    push rax
    retfq
vm86_return_long_stage2:
    ;  Restore IDTR
    lea rax, [edi + OFFSETOF(vm86_saved_idtr)]
    lidt [rax]
    ;  Restore stack pointer
    lea rax, [edi + OFFSETOF(vm86_saved_rsp)]
    mov rsp, [rax]
    ;  Restore EFER
    lea rax, [edi + OFFSETOF(vm86_saved_efer)]
    mov rax, [rax]
    WRITEMSR 0xC0000080
    ;  Restore base registers
    lea rax, [edi + OFFSETOF(vm86_saved_fsbase)]
    mov rax, [rax]
    WRITEMSR 0xC0000100
    lea rax, [edi + OFFSETOF(vm86_saved_gsbase)]
    mov rax, [rax]
    WRITEMSR 0xC0000101
    lea rax, [edi + OFFSETOF(vm86_saved_kgsbase)]
    mov rax, [rax]
    WRITEMSR 0xC0000102

    ;  Copy over the registers back to the kernel regs structure
    pop rax
    lea rbx, [edi + OFFSETOF(vm86_real_regs)]
    mov rcx, vm86_real_regs_end - vm86_real_regs
.copy_regs:
    mov rdx, [rbx],
    mov [rax], rdx
    inc rax
    inc rbx
    dec rcx
    jnz .copy_regs

    popf
    RESTORE_REGS_ALL
    sti
    ret

align 8
vm86_real_idtr:
    dw 0x03ff
    dd 0x0

align 8
vm86_real_gdt:
    dq 0x0000000000000000
    dq 0x000F9A000000FFFF   ; code 0x8
    dq 0x000F92000000FFFF   ; data 0x10
vm86_real_gdtr:
    dw $ - vm86_real_gdt - 1 ; size
    dd OFFSETOF(vm86_real_gdt)  ; offset, requires patching at runtime

align 8
vm86_compat_gdt:
    dq 0
    dq 0x00CF9A000000FFFF   ; ring0, code, 0x8
    dq 0x00CF92000000FFFF   ; ring0, data, 0x10
vm86_compat_gdtr:
    dw $ - vm86_compat_gdt - 1 ; size
    dq OFFSETOF(vm86_compat_gdt)  ; offset, requires patching at runtime

align 8
vm86_long_gdt:
    dq 0x0
    dq 0x00209A0000000000
    dq 0x0000920000000000
vm86_long_gdtr:
    dw $ - vm86_long_gdt - 1 ; size
    dq OFFSETOF(vm86_long_gdt)  ; offset, requires patching at runtime

align 8
vm86_saved_cr3:
    dq 0x0
align 8
vm86_saved_gdtr:
    dw 0x0
    dq 0x0
align 8
vm86_saved_idtr:
    dw 0x0
    dq 0x0
align 8
vm86_saved_rsp:
    dq 0x0
vm86_saved_fsbase:
    dq 0x0
vm86_saved_gsbase:
    dq 0x0
vm86_saved_kgsbase:
    dq 0x0
vm86_saved_efer:
    dq 0x0

align 8
vm86_real_regs:
    dw 0x0  ;  ax
    dw 0x0  ;  bx
    dw 0x0  ;  cx
    dw 0x0  ;  dx
    dw 0x0  ;  si
    dw 0x0  ;  di
    dw 0x0  ;  es
vm86_real_regs_end:

global vm86_shellcode_end
vm86_shellcode_end:

%if (vm86_shellcode_end-vm86_shellcode_start) > 4096
%error "VM86 shellcode size is larger than one page!"
%endif
