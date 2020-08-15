section .text

%macro define_entrypoint_for_exception 1
global _exception_entry_%{1:1}
_exception_entry_%{1:1}:
    pusha

;    mov eax, esp
;    push eax

    extern _kernel_exception_%{1:1}
    call _kernel_exception_%{1:1}

    pop eax

    popa
    iret
%endmacro

%macro define_entrypoint_for_exception_pop_error 1
global _exception_entry_%{1:1}
_exception_entry_%{1:1}:
    pusha

;    mov eax, esp
;    push eax

    extern _kernel_exception_%{1:1}
    call _kernel_exception_%{1:1}

    pop eax

    popa
    add esp, 4
    iret
%endmacro

define_entrypoint_for_exception divbyzero
define_entrypoint_for_exception dbg
define_entrypoint_for_exception nmi
define_entrypoint_for_exception break
define_entrypoint_for_exception overflow
define_entrypoint_for_exception bound
define_entrypoint_for_exception invalidop
define_entrypoint_for_exception nodevice
define_entrypoint_for_exception doublefault
define_entrypoint_for_exception invalidtss
define_entrypoint_for_exception invalidseg
define_entrypoint_for_exception segstackfault
define_entrypoint_for_exception gpf
define_entrypoint_for_exception pagefault
define_entrypoint_for_exception x87fpfault
define_entrypoint_for_exception aligncheck
define_entrypoint_for_exception machinecheck
define_entrypoint_for_exception simdfp
define_entrypoint_for_exception virtfault
define_entrypoint_for_exception security