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
	auto thread = Thread::create_in_process(Process::kerneld(), _kernel_idle_task);
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

		auto new_thread = Thread::create_in_process(new_process, []{
			auto* current = Thread::current();

			//  Create a userland stack for the thread
			void* user_stack = current->parent()->vmm().allocate_user_stack(VMM::user_stack_size());

			const uint8 bytes[] = { 0x48, 0xB8, 0x6F, 0x72, 0x6C, 0x64, 0x21, 0x00, 0x00, 0x00, 0x50, 0x48, 0xB8, 0x48, 0x65, 0x6C, 0x6C, 0x6F, 0x2C, 0x20, 0x77, 0x50, 0x48, 0x89, 0xE7, 0x48, 0xC7, 0xC0, 0xFF, 0x00, 0x00, 0x00, 0x0F, 0x05, 0x48, 0xC7, 0xC0, 0x64, 0x00, 0x00, 0x00, 0x48, 0xC7, 0xC7, 0x00, 0x40, 0x00, 0x00, 0x0F, 0x05, 0x48, 0xC7, 0xC3, 0xFF, 0xFF, 0xFF, 0xFF, 0x48, 0xC7, 0xC1, 0x00, 0x40, 0x00, 0x00, 0x48, 0x89, 0x18, 0x48, 0x83, 0xC0, 0x08, 0x48, 0x83, 0xE9, 0x08, 0x48, 0x83, 0xF9, 0x00, 0x75, 0xEF, 0x48, 0xC7, 0xC3, 0x0A, 0x00, 0x00, 0x00, 0x48, 0xC7, 0xC7, 0xE8, 0x03, 0x00, 0x00, 0x48, 0xC7, 0xC0, 0xFE, 0x00, 0x00, 0x00, 0x0F, 0x05, 0x48, 0xFF, 0xCB, 0x48, 0x83, 0xFB, 0x00, 0x75, 0xE7, 0x48, 0xC7, 0xC0, 0x64, 0x00, 0x00, 0x00, 0x48, 0xC7, 0xC7, 0x00, 0x10, 0x00, 0x00, 0x0F, 0x05, 0x48, 0xC7, 0xC3, 0xFF, 0xFF, 0xFF, 0xFF, 0x48, 0xC7, 0xC1, 0x00, 0x10, 0x00, 0x00, 0x48, 0x89, 0x18, 0x48, 0x83, 0xC0, 0x08, 0x48, 0x83, 0xE9, 0x08, 0x48, 0x83, 0xF9, 0x00, 0x75, 0xEF, 0x48, 0xC7, 0xC7, 0xE8, 0x03, 0x00, 0x00, 0x48, 0xC7, 0xC0, 0xFE, 0x00, 0x00, 0x00, 0x0F, 0x05, 0xEB, 0xEE };
			auto* shellcode_location = (uint8*)0x100000;

			kdebugf("Thread(%i): mapping shellcode\n", current->tid());
			auto mapping = VMapping::create((void*)shellcode_location, 0x1000, VM_READ | VM_WRITE | VM_EXEC, MAP_SHARED);
			kassert(current->parent()->vmm().insert_vmapping(gen::move(mapping)));

			kdebugf("Thread(%i): copying shellcode\n", current->tid());
			for(auto& b : bytes) {
				*shellcode_location = b;
				shellcode_location++;
			}

			kdebugf("Thread(%i): jumping to user\n", current->tid());
			PtraceRegs regs = PtraceRegs::user_default();
			regs.rip = 0x100000;
			regs.rsp = (uint64)user_stack;
			CPU::jump_to_user(&regs);
		});
		new_thread->sched_ctx().priority = 10;
		add_thread_to_rq(new_thread.get());
	}

	{
		auto new_process = Process::kerneld();
		auto new_thread = Thread::create_in_process(new_process, _kernel_test_task_2);
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

