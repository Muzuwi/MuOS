extern _ZN7Process16ensure_directoryEv
extern _ZN7Process19ensure_kernel_stackEv
extern _ZN7Process18_finalize_internalEv

global _ZN7Process8finalizeEv
_ZN7Process8finalizeEv:
    mov ebp, esp

    push dword [ebp+4]
    call _ZN7Process16ensure_directoryEv
    add esp, 4
    push eax    ;  Directory

    push dword [ebp+4]
    call _ZN7Process19ensure_kernel_stackEv
    add esp, 4
    push eax    ;  Kernel stack top

    mov eax, [esp+4]    ; Page directory
    mov ebx, [esp+0]    ; Kernel stack top
    mov ecx, [ebp+4]    ; Process pointer

    mov edx, cr3
    cmp edx, eax
    je __skip_cr3
    mov cr3, eax
__skip_cr3:
    invlpg [ebx]

    mov esp, ebx
    mov ebp, esp

    ;  Call finalization routines
    push ecx
    call _ZN7Process18_finalize_internalEv

    ;  Not reachable
    ud2
    cli
    hlt