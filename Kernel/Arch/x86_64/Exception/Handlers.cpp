#include <Arch/x86_64/CPU.hpp>
#include <Core/Log/Logger.hpp>
#include <Core/MP/MP.hpp>
#include <Process/Process.hpp>
#include <Process/Thread.hpp>
#include <Scheduler/Scheduler.hpp>
#include "Exception.hpp"

CREATE_LOGGER("x86_64::exception", core::log::LogLevel::Debug);

static char const* EXCEPTION_VECTOR_TO_STRING[32] = { "Division Error",
	                                                  "Debug",
	                                                  "Non-maskable Interrupt",
	                                                  "Breakpoint",
	                                                  "Overflow",
	                                                  "Bound Range Exceeded",
	                                                  "Invalid Opcode",
	                                                  "Device Not Available",
	                                                  "Double Fault",
	                                                  "Coprocessor Segment Overrun",
	                                                  "Invalid TSS",
	                                                  "Segment Not Present",
	                                                  "Stack-Segment Fault",
	                                                  "General Protection Fault",
	                                                  "Page Fault",
	                                                  "Reserved",
	                                                  "x87 Floating-Point Exception",
	                                                  "Alignment Check",
	                                                  "Machine Check",
	                                                  "SIMD Floating-Point Exception",
	                                                  "Virtualization Exception",
	                                                  "Control Protection Exception",
	                                                  "Reserved",
	                                                  "Reserved",
	                                                  "Reserved",
	                                                  "Reserved",
	                                                  "Reserved",
	                                                  "Reserved",
	                                                  "Hypervisor Injection Exception",
	                                                  "VMM Communication Exception",
	                                                  "Security Exception",
	                                                  "Reserved" };

static void dump_registers(PtraceRegs* pt) {
	log.error("rax={x} rbx={x} rcx={x} rdx={x}", pt->rax, pt->rbx, pt->rcx, pt->rdx);
	log.error("rsi={x} rdi={x} rbp={x} rip={x}", pt->rsi, pt->rdi, pt->rbp, pt->rip);
	log.error(" r8={x}  r9={x} r10={x} r11={x}", pt->r8, pt->r9, pt->r10, pt->r11);
	log.error("r12={x} r13={x} r14={x} r15={x}", pt->r12, pt->r13, pt->r14, pt->r15);
	log.error("rsp={x} rflags={x} cs={x} origin={x}", pt->rsp, pt->rflags, pt->cs, pt->origin);
	log.error("gsbase={x}  kgsbase={x}", CPU::get_gs_base(), CPU::get_kernel_gs_base());
}

Exception::Response Exception::handle_uncaught(PtraceRegs* pt, uint8 vector) {
	const auto thread = this_cpu()->current_thread();

	log.error("Unhandled exception vector={x} ({})", vector, EXCEPTION_VECTOR_TO_STRING[vector & 0x1F]);
	dump_registers(pt);

	if(!thread) {
		log.fatal("Kernel Panic - unhandled exception in kernel mode");
		return Response::KernelPanic;
	}

	if(thread->parent()->flags().privilege == Kernel) {
		log.fatal("Kernel Panic - unhandled exception in kernel-mode process");
		return Response::KernelPanic;
	}

	log.error("in thread(tid={}), process(pid={})", thread->tid(), thread->parent()->pid());
	log.error("task_frame={x}, stack_frame={x}", Format::ptr(thread->irq_task_frame()), Format::ptr(pt));
	log.error("Thread(tid={}): Uncaught exception - killing", thread->tid());
	thread->set_state(TaskState::Leaving);
	thread->reschedule();
	this_cpu()->scheduler->schedule();

	return Exception::Response::TerminateThread;
}

Exception::Response Exception::handle_page_fault(PtraceRegs* pt, uint8) {
	const auto thread = this_cpu()->current_thread();

	dump_registers(pt);

	if(!thread) {
		log.fatal("Kernel Panic - page fault in kernel mode");
		return Response::KernelPanic;
	}

	if(thread->parent()->flags().privilege == User) {
		log.error("Thread(tid={}): Page fault - killing", thread->tid());
		return Response::TerminateThread;
	}

	log.fatal("Kernel Panic - page fault in kernel-mode process");
	return Exception::Response::KernelPanic;
}
