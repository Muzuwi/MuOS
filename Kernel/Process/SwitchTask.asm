section .text
global _ZN9Scheduler11switch_taskEv
_ZN9Scheduler11switch_taskEv:
    pushfd
    pushad      ;  Store register state

    push esp    ;  Do yield
    extern _ZN9Scheduler20yield_with_irq_frameEj
    call _ZN9Scheduler20yield_with_irq_frameEj

    ;   Not reachable
    ud2
    cli
    hlt