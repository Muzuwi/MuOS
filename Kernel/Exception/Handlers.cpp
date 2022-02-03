#include <Arch/x86_64/CPU.hpp>
#include <Debug/klogf.hpp>
#include <Kernel/Exception/Exception.hpp>
#include <Kernel/Process/Process.hpp>
#include <Kernel/Process/Thread.hpp>
#include <Kernel/SMP/SMP.hpp>

static void dump_registers(PtraceRegs* pt) {
	kerrorf_static("rax={x} rbx={x} rcx={x} rdx={x}\n", pt->rax, pt->rbx, pt->rcx, pt->rdx);
	kerrorf_static("rsi={x} rdi={x} rbp={x} rip={x}\n", pt->rsi, pt->rdi, pt->rbp, pt->rip);
	kerrorf_static(" r8={x}  r9={x} r10={x} r11={x}\n", pt->r8, pt->r9, pt->r10, pt->r11);
	kerrorf_static("r12={x} r13={x} r14={x} r15={x}\n", pt->r12, pt->r13, pt->r14, pt->r15);
	kerrorf_static("rsp={x} rflags={x} cs={x} origin={x}\n", pt->rsp, pt->rflags, pt->cs, pt->origin);
	kerrorf_static("gsbase={x}  kgsbase={x}\n", CPU::get_gs_base(), CPU::get_kernel_gs_base());
}

Exception::Response Exception::handle_uncaught(PtraceRegs* pt, uint8 vector) {
	const auto thread = SMP::ctb().current_thread();

	kerrorf_static("[Exception] Unhandled exception vector={x}\n", vector);
	dump_registers(pt);

	if(!thread) {
		kerrorf_static("Kernel Panic - unhandled exception in kernel mode\n");
		return Response::KernelPanic;
	}

	if(thread->parent()->flags().privilege == Kernel) {
		kerrorf_static("Kernel Panic - unhandled exception in kernel-mode process\n");
		return Response::KernelPanic;
	}

	kerrorf_static("in thread(tid={}), process(pid={})\n", thread->tid(), thread->parent()->pid());
	kerrorf_static("task_frame={x}, stack_frame={x}\n", Format::ptr(thread->irq_task_frame()), Format::ptr(pt));
	kerrorf_static("Thread(tid={}): Uncaught exception - killing\n", thread->tid());
	thread->set_state(TaskState::Leaving);
	thread->reschedule();
	SMP::ctb().scheduler().schedule();

	return Exception::Response::TerminateThread;
}


Exception::Response Exception::handle_page_fault(PtraceRegs* pt, uint8) {
	const auto thread = SMP::ctb().current_thread();

	dump_registers(pt);

	if(!thread) {
		kerrorf_static("Kernel Panic - page fault in kernel mode\n");
		return Response::KernelPanic;
	}

	if(thread->parent()->flags().privilege == User) {
		kerrorf_static("Thread(tid={}): Page fault - killing\n", thread->tid());
		return Response::TerminateThread;
	}

	kerrorf_static("Kernel Panic - page fault in kernel-mode process\n");
	return Exception::Response::KernelPanic;
}
