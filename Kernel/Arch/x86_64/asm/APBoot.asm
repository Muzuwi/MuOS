%include "BootDefines.mac"

bits 64
section .text

struc APBootstrap
.state_flag resq 1
.cr3        resq 1
.rsp        resq 1
.code_page  resq 1
.data_page  resq 1
.real_gdt        resq 3
.real_gdtr_size  resw 1
.real_gdtr_off   resd 1
.compat_gdt        resq 3
.compat_gdtr_size  resw 1
.compat_gdtr_off   resq 1
.long_gdt        resq 3
.long_gdtr_size  resw 1
.long_gdtr_off   resq 1
.kernel_idtr_size   resw 1
.kernel_idtr_off    resq 1
.ap_ctb     resq 1
.idle_task  resq 1
endstruc

align 4096
bits 16
global ap_bootstrap_start
ap_bootstrap_start:
%define OFFSETOF(a) (a - ap_bootstrap_start)
    cli
    mov di, 0x5555  ;  This is patched by the kernel to contain the data page,
                    ;  it's not actually 0x5555
                    ;  DO NOT MOVE THIS AROUND!

    ;  Signal to the BSP that we're up
    mov ax, 0x1
    mov [edi], ax

    ;  Enable protection
    mov eax, cr0
    or eax, (1 << 0)
    mov cr0, eax
    ;  Load 16-bit protected mode GDTR
    lea eax, [edi + APBootstrap.real_gdtr_size]
    lgdt [eax]
    ;  Use data page as stack
    lea eax, [edi + 0x1000]
    mov esp, eax
    mov ebp, esp

    mov si, cs
    shl si, 4   ;  cs * 16 = address of code page
    lea eax, [esi + OFFSETOF(ap_prot16)]

    push dword 0x8
    push eax
    o32 retf
ap_prot16:
    ;  Load proper selectors
    mov ax, 0x10
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov ss, ax
    ;  Load 32-bit mode GDTR
    lea eax, [edi + APBootstrap.compat_gdtr_size]
    lgdt [eax]
    ;  Far jump to 32-bit mode
    lea eax, [esi + OFFSETOF(ap_prot32)]
    push dword 0x8
    push eax
    o32 retf
bits 32
ap_prot32:
    ;  Enable PAE/PGE
    mov eax, CR4_PAE|CR4_PGE|CR4_PSE
    mov cr4, eax
    ;  Load target CR3
    lea eax, [edi + APBootstrap.cr3]
    mov eax, [eax]
    mov cr3, eax
    ;  Enable long mode support in EFER, and NX when supported
    ;  We have to enable NX immediately, otherwise using paging structures containing
    ;  NX bits is undefined.
    mov eax, 0x80000001
    cpuid
    and edx, 1 << 20
    jz .no_nxe
    mov eax, EFER_LM | EFER_NXE
    push eax
    jmp .write_efer
.no_nxe:
    mov eax, EFER_LM
    push eax
.write_efer:
    mov ecx, 0xc0000080
    rdmsr
    pop ebx
    or eax, ebx
    wrmsr
    ;  Enable PE/PG
    mov eax, cr0
    or eax, CR0_PE|CR0_PG
    mov cr0, eax
    ;  Load long-mode GDT
    lea eax, [edi + APBootstrap.long_gdtr_size]
    lgdt [eax]
    ;  Far ret into long mode
    lea eax, [esi + OFFSETOF(ap_long)]
    push dword 0x8
    push eax
    retf
bits 64
ap_long:
    ;  Far ret into kernel address space
    mov rax, ap_long_entry
    push rax
    ret
global ap_bootstrap_end
ap_bootstrap_end:

bits 64
global ap_long_entry
ap_long_entry:
    lea rax, [edi + APBootstrap.rsp]
    mov rax, [rax]
    mov rsp, rax
    mov rbp, rax

    mov rax, [edi + APBootstrap.data_page]
    push rax
    mov rax, [edi + APBootstrap.code_page]
    push rax
    mov rax, [edi + APBootstrap.idle_task]
    push rax
    mov rax, [edi + APBootstrap.ap_ctb]
    push rax

    ;  Signal the BSP that we're out of bootstrap
    lea rax, [edi + APBootstrap.state_flag]
    mov rbx, 2
    mov [rax], rbx

    ;  Restore function arguments
    pop rdi
    pop rsi
    pop rdx
    pop rcx

    ;  FIXME: Looks nasty, but friend definitions don't like extern "C" functions, so use a static class func instead
    extern _ZN3SMP13ap_entrypointEP12ControlBlockP6Thread8PhysAddrS4_
    call _ZN3SMP13ap_entrypointEP12ControlBlockP6Thread8PhysAddrS4_
.halt:
    hlt
    jmp .halt

%if (ap_bootstrap_end-ap_bootstrap_start) > 4096
%error "AP bootstrap code size is larger than one page!"
%endif
