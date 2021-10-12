## SMP/Kernel Preempt Compatibility

Places where preemptability in the kernel needs to be handled:

    - IRQ - not needed - already running in atomic context (IRQ disabled), 
    however no locks need to be taken (guaranteed deadlocks if multiple 
    IRQ contexts lock each other)

    - During syscalls - need handling, can be preempted at any time 

    - During exceptions - maybe not? should exceptions be preemptable?

### IRQ

**Do not** take any locks in IRQ context, else a deadlock is only a matter of time.


### Syscalls

Each syscall starts at ```_ukernel_syscall_entry```, and is then dispatched to different handlers. This can be interrupted at any time, unless preemption was temporarily disabled (via ```preempt_disable```)


### Exceptions

Not a concern yet? They're running with IRQs disabled









