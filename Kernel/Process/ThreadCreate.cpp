#include <string.h>
#include <Arch/i386/CPU.hpp>
#include <Arch/i386/GDT.hpp>
#include <Arch/i386/InactiveTaskFrame.hpp>
#include <Kernel/Process/Process.hpp>
#include <Kernel/Process/Thread.hpp>
#include <Kernel/Process/PidAllocator.hpp>
#include <Kernel/SMP/SMP.hpp>


SharedPtr<Thread> Thread::create_in_process(SharedPtr<Process> parent) {
	auto thread = SharedPtr {
		new (KHeap::allocate(sizeof(Thread))) Thread {parent, PidAllocator::next()}
	};

	parent->add_thread(thread);

	return thread;
}


[[maybe_unused]]
void Thread::finalize_switch(Thread* prev, Thread* next) {
	if(prev->state() == TaskState::Running)
		prev->set_state(TaskState::Ready);

	//  Save previous process' kernel GS base (userland GSbase when task is a ring3 task,
	//  and unused GSbase for kernel threads)
	prev->m_kernel_gs_base = CPU::get_kernel_gs_base();

	//  Set new process as current in CTB
	SMP::ctb().set_thread(next);

	//  Reset IRQ stack in the TSS
	GDT::set_irq_stack(next->m_kernel_stack_bottom);

	//  Restore saved kernel gs base of next process
	CPU::set_kernel_gs_base((void*)next->m_kernel_gs_base);

	//  Set the GS base to point to the current AP's CTB
	//  FIXME: Realistically only need to do this once, at AP boot
	CPU::set_gs_base((void*)(&SMP::ctb()));
}


void* Thread::_bootstrap_task_stack(PhysAddr kernel_stack_bottom, PtraceRegs state) {
	uint8* kernel_stack = kernel_stack_bottom.as<uint8>().get_mapped();
	auto struct_begin = kernel_stack - sizeof(PtraceRegs);

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

	auto offset = (uint8*)m_kernel_stack_bottom - sizeof(PtraceRegs) - sizeof(uint64_t) - sizeof(InactiveTaskFrame);
	return offset;
}
