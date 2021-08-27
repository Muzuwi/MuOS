#include <Kernel/Exception/Exception.hpp>
#include <Kernel/Debug/kdebugf.hpp>
#include <Kernel/Debug/kpanic.hpp>
#include <Kernel/SMP/SMP.hpp>
#include <Kernel/Process/Thread.hpp>
#include <Kernel/Process/Process.hpp>
#include <Arch/i386/CPU.hpp>

static Exception::HandlerFunction s_exception_handlers[32] {
		nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr,
		nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr,
		nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr,
		nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr
};

#define S(a) (uintptr_t)a>>32u, (uintptr_t)a&0xffffffffu

uint64 read_cr2() {
	uint64 data = 0;
	asm volatile(
			"mov %0, %%cr2"
			:"=r"(data)
			:);
	return data;
}

extern "C" void _kernel_exception_entrypoint(size_t vector, PtraceRegs* interrupt_stack_frame) {
	auto* pt = interrupt_stack_frame;
	kerrorf("[Exception] vector=%x\n", vector);
	kerrorf("rax=%x%x rbx=%x%x rcx=%x%x rdx=%x%x\n", S(pt->rax), S(pt->rbx),S(pt->rcx),S(pt->rdx));
	kerrorf("rsi=%x%x rdi=%x%x rbp=%x%x rip=%x%x\n", S(pt->rsi), S(pt->rdi),S(pt->rbp),S(pt->rip));
	kerrorf(" r8=%x%x  r9=%x%x r10=%x%x r11=%x%x\n", S(pt->r8), S(pt->r9),S(pt->r10),S(pt->r11));
	kerrorf("r12=%x%x r13=%x%x r14=%x%x r15=%x%x\n", S(pt->r12), S(pt->r13),S(pt->r14),S(pt->r15));
	kerrorf("rsp=%x%x rflags=%x%x cs=%i origin=%x%x\n", S(pt->rsp),S(pt->rflags), pt->cs, S(pt->origin));
	kerrorf("gsbase=%x%x  kgsbase=%x%x\n", S(CPU::get_gs_base()), S(CPU::get_kernel_gs_base()));
	kerrorf("cr2=%x%x\n", read_cr2());

	const auto thread = SMP::ctb().current_thread();
	if(thread) {
		kerrorf("in thread(tid=%i), process(pid=%i)\n", thread->tid(), thread->parent()->pid());
		kerrorf("task_frame=%x%x, stack_frame=%x%x\n", S(thread->irq_task_frame()), S(interrupt_stack_frame));
	}

	if(!s_exception_handlers[vector]) {
		kerrorf("[Exception] Unhandled exception vector=%x\n", vector);

		if(!thread) {
			kerrorf("Panic - unhandled exception in kernel mode\n");
			kpanic();
		}

		if(thread->parent()->flags().privilege == Kernel) {
			kerrorf("Panic - unhandled exception in kernel mode process\n");
			kpanic();
		}

		kerrorf("Thread(%i): Uncaught exception - killing\n");
		thread->set_state(TaskState::Leaving);
		thread->reschedule();
		SMP::ctb().scheduler().schedule();
	}

	auto handler = reinterpret_cast<Exception::HandlerFunction>(s_exception_handlers[vector]);
	auto response = handler(interrupt_stack_frame);
	if(response == Exception::Response::KernelPanic) {
		kpanic();
	} else if(response == Exception::Response::TerminateProcess) {
		//  Terminate process
	}
}