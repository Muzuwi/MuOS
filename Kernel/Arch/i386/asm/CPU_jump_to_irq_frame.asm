;   Jumps to an IRQ frame passed on the stack
global _ZN3CPU17jump_to_irq_frameEPvS0_
_ZN3CPU17jump_to_irq_frameEPvS0_:
    mov eax, [esp+4]    ; IRQ frame
    mov ebx, [esp+8]    ; cr3
    mov ecx, cr3

    cmp ebx, ecx
    je __skip_cr3_load
    mov cr3, ebx
__skip_cr3_load:
    mov esp, eax
    invlpg [eax]

    popad
    popfd

    ret
