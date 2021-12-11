#include <Arch/x86_64/CPU.hpp>
#include <Kernel/Debug/kdebugf.hpp>
#include <Kernel/Exception/Exception.hpp>
#include <Kernel/Process/Process.hpp>
#include <Kernel/Process/Thread.hpp>
#include <Kernel/SMP/SMP.hpp>


#define S(a) (uintptr_t)a>>32u, (uintptr_t)a&0xffffffffu

static void dump_registers(PtraceRegs* pt) {
	kerrorf("rax=%x%x rbx=%x%x rcx=%x%x rdx=%x%x\n", S(pt->rax), S(pt->rbx),S(pt->rcx),S(pt->rdx));
	kerrorf("rsi=%x%x rdi=%x%x rbp=%x%x rip=%x%x\n", S(pt->rsi), S(pt->rdi),S(pt->rbp),S(pt->rip));
	kerrorf(" r8=%x%x  r9=%x%x r10=%x%x r11=%x%x\n", S(pt->r8), S(pt->r9),S(pt->r10),S(pt->r11));
	kerrorf("r12=%x%x r13=%x%x r14=%x%x r15=%x%x\n", S(pt->r12), S(pt->r13),S(pt->r14),S(pt->r15));
	kerrorf("rsp=%x%x rflags=%x%x cs=%i origin=%x%x\n", S(pt->rsp),S(pt->rflags), pt->cs, S(pt->origin));
	kerrorf("gsbase=%x%x  kgsbase=%x%x\n", S(CPU::get_gs_base()), S(CPU::get_kernel_gs_base()));
}

Exception::Response Exception::handle_uncaught(PtraceRegs* pt, uint8 vector) {
	const auto thread = SMP::ctb().current_thread();

	kerrorf("[Exception] Unhandled exception vector=%x\n", vector);
	dump_registers(pt);

	if(!thread) {
		kerrorf("Kernel Panic - unhandled exception in kernel mode\n");
		return Response::KernelPanic;
	}

	if(thread->parent()->flags().privilege == Kernel) {
		kerrorf("Kernel Panic - unhandled exception in kernel-mode process\n");
		return Response::KernelPanic;
	}

	kerrorf("in thread(tid=%i), process(pid=%i)\n", thread->tid(), thread->parent()->pid());
	kerrorf("task_frame=%x%x, stack_frame=%x%x\n", S(thread->irq_task_frame()), S(pt));
	kerrorf("Thread(tid=%i): Uncaught exception - killing\n", thread->tid());
	thread->set_state(TaskState::Leaving);
	thread->reschedule();
	SMP::ctb().scheduler().schedule();

	return Exception::Response::TerminateThread;
}


Exception::Response Exception::handle_page_fault(PtraceRegs* pt, uint8) {
	const auto thread = SMP::ctb().current_thread();

	dump_registers(pt);

	if(!thread) {
		kerrorf("Kernel Panic - page fault in kernel mode\n");
		return Response::KernelPanic;
	}

	if(thread->parent()->flags().privilege == User) {
		kerrorf("Thread(tid=%i): Page fault - killing\n", thread->tid());
		return Response::TerminateThread;
	}

	kerrorf("Kernel Panic - page fault in kernel-mode process\n");
	return Exception::Response::KernelPanic;
}
