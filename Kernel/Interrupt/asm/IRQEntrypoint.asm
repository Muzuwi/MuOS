section .text

global _kernel_syscall_entry
_kernel_syscall_entry:
    cli
    pusha
    extern _ukernel_syscall_handler
    call _ukernel_syscall_handler
    popa
    sti
    iret

%macro define_entrypoint_for_irq 1
global irq%{1:1}
irq%{1:1}:
    cli
    pusha
    push dword %{1:1}   ;  Push IRQ number
    extern _kernel_irq_dispatch
    call _kernel_irq_dispatch
    pop eax             ;  Pop IRQ number
    popa
    sti
    iret
%endmacro

define_entrypoint_for_irq 0
define_entrypoint_for_irq 1
define_entrypoint_for_irq 2
define_entrypoint_for_irq 3
define_entrypoint_for_irq 4
define_entrypoint_for_irq 5
define_entrypoint_for_irq 6
define_entrypoint_for_irq 7
define_entrypoint_for_irq 8
define_entrypoint_for_irq 9
define_entrypoint_for_irq 10
define_entrypoint_for_irq 11
define_entrypoint_for_irq 12
define_entrypoint_for_irq 13
define_entrypoint_for_irq 14
define_entrypoint_for_irq 15

