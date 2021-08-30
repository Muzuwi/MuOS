#include <Arch/i386/CPU.hpp>
#include <Arch/i386/GDT.hpp>
#include <Kernel/Scheduler/RunQueue.hpp>
#include <Kernel/Scheduler/Scheduler.hpp>
#include <Kernel/Process/Thread.hpp>
#include <Kernel/Process/Process.hpp>
#include <Kernel/Debug/kassert.hpp>
#include <Kernel/SMP/SMP.hpp>


[[noreturn]] void _kernel_idle_task() {
	while(true)
		asm volatile("hlt");
}

[[noreturn]] void _kernel_test_task_2() {
	while(true) {
		kdebugf("test thread2, pid=%i, tid=%i\n", Thread::current()->parent()->pid(), Thread::current()->tid());
		Thread::current()->msleep(2000);
	}
}


void Scheduler::tick() {
	auto* thread = SMP::ctb().current_thread();
	if(!thread)
		return;

	if(thread == m_ap_idle) {
		//  Force reschedule when at least one thread becomes runnable
		if(m_rq.find_runnable() != nullptr)
			thread->reschedule();
		else
			thread->clear_reschedule();
		return;
	}

	//  Cannot preempt, process is holding locks
	if(thread->preempt_count() > 0) {
		return;
	}

	//  Spend one process quantum
	if(thread->sched_ctx().quants_left) {
		thread->sched_ctx().quants_left--;
	} else {
		//  Reschedule currently running thread - ran out of quants
		thread->reschedule();
		thread->sched_ctx().quants_left = pri_to_quants(120 + thread->priority());
	}
}


/*
 *  Called when a task voluntarily relinquishes its' CPU time
 */
void Scheduler::schedule() {
	auto* thread = SMP::ctb().current_thread();
	thread->preempt_disable();

	//  Find next runnable task
	auto* next_thread = m_rq.find_runnable();
	if(!next_thread)
		next_thread = m_ap_idle;

	CPU::switch_to(thread, next_thread);
	thread->preempt_enable();
}


void Scheduler::interrupt_return_common() {
	auto* current = SMP::ctb().current_thread();
	if(!current)
		return;
	if(!current->needs_reschedule())
		return;

	current->clear_reschedule();
	schedule();
}


void Scheduler::wake_up(Thread* thread) {
	if(!thread) return;

	assert(thread->state() != TaskState::Running);
	thread->set_state(TaskState::Ready);

	auto* current = SMP::ctb().current_thread();
	if(thread->priority() < current->priority())
		current->reschedule();
}


Thread* Scheduler::create_idle_task() {
	auto thread = Thread::create_in_process(Process::kerneld());
	auto& kerneld = *Process::kerneld();

	auto stack_mapping = kerneld.vmm().allocate_kernel_stack(VMM::kernel_stack_size());
	void* const stack_top = stack_mapping->addr();
	void* const stack_bottom = (void*)((uintptr_t)stack_mapping->addr() + VMM::kernel_stack_size());

	auto stack_last_page = stack_mapping->page_for((void*)((uintptr_t)stack_bottom-1)).unwrap();

	thread->m_pml4 = Process::kerneld()->vmm().m_pml4;
	thread->m_kernel_stack_bottom = stack_bottom;

	PtraceRegs state0 {};
	state0.rip = (uint64)&_kernel_idle_task;
	state0.cs = GDT::get_kernel_CS();
	state0.ss = GDT::get_kernel_DS();
	state0.rflags = 0x0200;
	state0.rsp = (uint64)stack_bottom;
	state0.rbp = (uint64)stack_bottom;
	thread->m_interrupted_task_frame = (InactiveTaskFrame*)thread->_bootstrap_task_stack(
			PhysAddr{(stack_last_page+1).get()}, state0
			);

	return thread.get();
}


/*
 *  Creates the idle task for the current AP and enters it, kickstarting the scheduler on the current AP
 */
void Scheduler::bootstrap() {
	CPU::irq_disable();
	kdebugf("[Scheduler] Bootstrapping AP %i\n", SMP::ctb().current_ap());

	m_ap_idle = create_idle_task();

	{
		auto new_process = Process::init();
		new_process->vmm().m_pml4 = new_process->vmm().clone_pml4(Process::kerneld()->vmm().m_pml4).unwrap();

		auto new_thread = Thread::create_in_process(new_process);

		auto stack_mapping = new_process->vmm().allocate_kernel_stack(VMM::kernel_stack_size());
		void* const stack_top = stack_mapping->addr();
		void* const stack_bottom = (void*)((uintptr_t)stack_mapping->addr() + VMM::kernel_stack_size());
		auto stack_last_page = stack_mapping->page_for((void*)((uintptr_t)stack_bottom-1)).unwrap();

		void* user_stack = new_process->vmm().allocate_user_stack(VMM::user_stack_size());

		auto mapping = VMapping::create((void*)0x100000, 0x1000, VM_READ | VM_WRITE | VM_EXEC, MAP_SHARED);
		auto page = mapping->page_for((void*)0x100000).unwrap();
		kassert(new_process->vmm().insert_vmapping(gen::move(mapping)));
		uint8 bytes[] = { 0x48, 0xB8, 0x6F, 0x72, 0x6C, 0x64, 0x21, 0x00, 0x00, 0x00, 0x50, 0x48, 0xB8, 0x48, 0x65, 0x6C, 0x6C, 0x6F, 0x2C, 0x20, 0x77, 0x50, 0x48, 0x89, 0xE7, 0x48, 0xC7, 0xC0, 0xFF, 0x00, 0x00, 0x00, 0x0F, 0x05, 0x48, 0xC7, 0xC7, 0xE8, 0x03, 0x00, 0x00, 0x48, 0xC7, 0xC0, 0xFE, 0x00, 0x00, 0x00, 0x0F, 0x05, 0xEB, 0xEE } ;
//		uint8 bytes[] = { 0x48, 0xB8, 0x6F, 0x72, 0x6C, 0x64, 0x21, 0x00, 0x00, 0x00, 0x50, 0x48, 0xB8, 0x48, 0x65, 0x6C, 0x6C, 0x6F, 0x2C, 0x20, 0x77, 0x50, 0x48, 0x89, 0xE7, 0x48, 0xC7, 0xC0, 0xFF, 0x00, 0x00, 0x00, 0x0F, 0x05, 0x48, 0xC7, 0xC7, 0xE8, 0x03, 0x00, 0x00, 0x48, 0xC7, 0xC0, 0xFE, 0x00, 0x00, 0x00, 0x0F, 0x05, 0x48, 0xC7, 0xC0, 0x00, 0x00, 0x00, 0x00, 0x48, 0x8B, 0x00 };
		for(auto& b : bytes) {
			*page = b;
			page++;
		}

		new_thread->m_kernel_stack_bottom = stack_bottom;
		PtraceRegs state {};
		state.rip = 0x100000;
		state.cs = GDT::get_user_CS() | 3;
		state.ss = GDT::get_user_DS() | 3;
		state.rflags = 0x0200;
		state.rsp = (uint64)user_stack;
		state.rbp = (uint64)user_stack;
		new_thread->m_interrupted_task_frame = (InactiveTaskFrame*)new_thread->_bootstrap_task_stack(
				PhysAddr{(stack_last_page+1).get()}, state
				);
		new_thread->sched_ctx().priority = 10;
		new_thread->set_state(TaskState::Ready);
		new_thread->m_pml4 = new_process->vmm().m_pml4;

		add_thread_to_rq(new_thread.get());
	}


	{
		auto new_process = Process::kerneld();
		auto new_thread = Thread::create_in_process(new_process);

		auto stack_mapping = new_process->vmm().allocate_kernel_stack(VMM::kernel_stack_size());
		void* const stack_top = stack_mapping->addr();
		void* const stack_bottom = (void*)((uintptr_t)stack_mapping->addr() + VMM::kernel_stack_size());

		auto stack_last_page = stack_mapping->page_for((void*)((uintptr_t)stack_bottom-1)).unwrap();

		new_thread->m_kernel_stack_bottom = stack_bottom;
		PtraceRegs state {};
		state.rip = (uint64)&_kernel_test_task_2;
		state.cs = GDT::get_kernel_CS();
		state.ss = GDT::get_kernel_DS();
		state.rflags = 0x0200;
		state.rsp = (uint64)stack_bottom;
		state.rbp = (uint64)stack_bottom;
		new_thread->m_interrupted_task_frame = (InactiveTaskFrame*)new_thread->_bootstrap_task_stack(
				PhysAddr{(stack_last_page+1).get()}, state
				);
		new_thread->sched_ctx().priority = 1;
		new_thread->set_state(TaskState::Ready);
		new_thread->m_pml4 = new_process->vmm().m_pml4;

		add_thread_to_rq(new_thread.get());
	}

	uint8 _dummy_val[sizeof(Thread)] = {};
	//  Huge hack - use dummy buffer on the stack when switching for the first time,
	//  as we don't care about saving garbage data
	CPU::switch_to(reinterpret_cast<Thread*>(&_dummy_val), m_ap_idle);
}

unsigned Scheduler::pri_to_quants(uint8_t priority) {
	kassert(priority <= 140);

	if(priority < 120)
		return (140 - priority) * 20;
	else
		return (140 - priority) * 5;
}

void Scheduler::add_thread_to_rq(Thread* thread) {

	//  FIXME/SMP: Locking
	m_rq.add(thread);

}

