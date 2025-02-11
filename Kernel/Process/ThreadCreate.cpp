#include <Arch/x86_64/CPU.hpp>
#include <Arch/x86_64/GDT.hpp>
#include <Arch/x86_64/InactiveTaskFrame.hpp>
#include <Arch/x86_64/MP/ExecutionEnvironment.hpp>
#include <Core/Mem/Heap.hpp>
#include <Core/Mem/VM.hpp>
#include <Core/MP/MP.hpp>
#include <Process/PidAllocator.hpp>
#include <Process/Process.hpp>
#include <Process/Thread.hpp>
#include <string.h>
#include <SystemTypes.hpp>

static void* _bootstrap_task_stack(uint8* kernel_stack_bottom, PtraceRegs state) {
	auto struct_begin = kernel_stack_bottom - sizeof(PtraceRegs);

	auto* regs = reinterpret_cast<PtraceRegs*>(struct_begin);
	memset(regs, 0x0, sizeof(PtraceRegs));

	//  We're modifying the kernel stack as follows:
	//  ----  stack bottom  ----
	//  <ptrace regs>
	//  <ret rip pointing to _task_enter_bootstrap>
	//  rbp
	//  rbx
	//  r12
	//  r13
	//  r14
	//  r15   <--- rsp

	*regs = state;

	auto* kernel_return_addr = struct_begin - sizeof(uint64_t);
	*((uint64_t*)kernel_return_addr) = (uint64_t)&_task_enter_bootstrap;

	auto* inactive_struct = kernel_return_addr - sizeof(InactiveTaskFrame);
	memset(inactive_struct, 0x0, sizeof(InactiveTaskFrame));

	auto offset = (uint8*)kernel_stack_bottom - sizeof(PtraceRegs) - sizeof(uint64_t) - sizeof(InactiveTaskFrame);
	return offset;
}

SharedPtr<Thread> Thread::create_in_process(SharedPtr<Process> parent, void (*kernel_exec)()) {
	auto thread = SharedPtr {
		new(core::mem::hmalloc(sizeof(Thread))) Thread { parent, PidAllocator::next() }
	};

	if(!thread) {
		return {};
	}

	parent->add_thread(thread);
	auto* stack_top = core::mem::vmalloc(VMM::kernel_stack_size());
	if(!stack_top) {
		return {};
	}

	auto* stack_bottom = reinterpret_cast<uint8*>(stack_top) + VMM::kernel_stack_size();
	thread->m_kernel_stack_bottom = stack_bottom;
	PtraceRegs state = PtraceRegs::kernel_default();
	state.rip = (uint64)kernel_exec;
	state.rsp = (uint64)stack_bottom;
	state.rbp = (uint64)stack_bottom;
	thread->m_interrupted_task_frame = (InactiveTaskFrame*)_bootstrap_task_stack(stack_bottom, state);
	thread->sched_ctx().priority = 1;
	thread->set_state(TaskState::Ready);
	thread->m_paging_handle = parent->vmm().paging_handle();

	return thread;
}

[[maybe_unused]] void Thread::finalize_switch(Thread* prev, Thread* next) {
	if(prev->state() == TaskState::Running) {
		prev->set_state(TaskState::Ready);
	}

	//  Save previous process' kernel GS base (userland GSbase when task is a ring3 task,
	//  and unused GSbase for kernel threads)
	prev->m_kernel_gs_base = CPU::get_kernel_gs_base();

	//  Set new process as current in CTB
	this_cpu()->set_thread(next);

	//  x86_64: Reset IRQ stack in the TSS
	this_cpu()->platform.tss.rsp0 = next->m_kernel_stack_bottom;
	//  x86_64: Set the thread pointer in ExecutionEnvironment
	this_cpu()->platform.thread = next;

	//  Restore saved kernel gs base of next process
	CPU::set_kernel_gs_base((void*)next->m_kernel_gs_base);

	next->set_state(TaskState::Running);
}
