#include <Arch/x86_64/CPU.hpp>
#include <Arch/x86_64/GDT.hpp>
#include <Kernel/Scheduler/Scheduler.hpp>
#include <Kernel/Process/Thread.hpp>
#include <Kernel/Process/Process.hpp>
#include <Kernel/Debug/kassert.hpp>
#include <Kernel/SMP/SMP.hpp>
#include <Kernel/Interrupt/IRQDispatcher.hpp>
#include <Debug/klogf.hpp>
#include "Daemons/BootAP/BootAP.hpp"
#include "Daemons/Idle/Idle.hpp"
#include "Daemons/Kbd/Kbd.hpp"
#include "Daemons/SysDbg/SysDbg.hpp"
#include "Daemons/Testd/Testd.hpp"

void Scheduler::tick() {
	auto* thread = SMP::ctb().current_thread();
	if(!thread) {
		return;
	}

	if(thread == m_ap_idle) {
		//  Force reschedule when at least one thread becomes runnable
		if(m_rq.find_runnable() != nullptr) {
			thread->reschedule();
		} else {
			thread->clear_reschedule();
		}
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
	if(!next_thread) {
		next_thread = m_ap_idle;
	}

	CPU::switch_to(thread, next_thread);
	thread->preempt_enable();
}


void Scheduler::interrupt_return_common() {
	auto* current = SMP::ctb().current_thread();
	if(!current) {
		return;
	}
	if(!current->needs_reschedule()) {
		return;
	}

	current->clear_reschedule();
	schedule();
}


void Scheduler::wake_up(Thread* thread) {
	if(!thread) { return; }

	assert(thread->state() != TaskState::Running);
	thread->set_state(TaskState::Ready);

	auto* current = SMP::ctb().current_thread();
	if(thread->priority() < current->priority()) {
		current->reschedule();
	}
}


Thread* Scheduler::create_idle_task() {
	auto thread = Thread::create_in_process(Process::kerneld(), Idle::idle_thread);
	return thread.get();
}


/*
 *  Creates the idle task for the current AP and enters it, kickstarting the scheduler on the current AP
 */
void Scheduler::bootstrap() {
	CPU::irq_disable();
	klogf("[Scheduler] Bootstrapping AP {}\n", SMP::ctb().current_ap());

	m_ap_idle = create_idle_task();

	{
		auto new_process = Process::init();
		new_process->vmm().m_pml4 = new_process->vmm().clone_pml4(Process::kerneld()->vmm().m_pml4).unwrap();

		auto new_thread = Thread::create_in_process(new_process, Testd::userland_test_thread);
		new_thread->sched_ctx().priority = 10;
		add_thread_to_rq(new_thread.get());
	}

	{
		auto new_process = Process::kerneld();
		auto new_thread = Thread::create_in_process(new_process, Testd::test_kernel_thread);
		add_thread_to_rq(new_thread.get());
	}

	{
		auto keyboard_thread = Thread::create_in_process(Process::kerneld(), Kbd::kbd_thread);
		add_thread_to_rq(keyboard_thread.get());
		IRQDispatcher::register_microtask(1, Kbd::kbd_microtask);
	}

	{
		auto ap_start_thread = Thread::create_in_process(Process::kerneld(), BootAP::boot_ap_thread);
		add_thread_to_rq(ap_start_thread.get());
	}

	{
		auto debugger_thread = Thread::create_in_process(Process::kerneld(), SysDbg::sysdbg_thread);
		add_thread_to_rq(debugger_thread.get());
	}

	uint8 _dummy_val[sizeof(Thread)] = {};
	//  Huge hack - use dummy buffer on the stack when switching for the first time,
	//  as we don't care about saving garbage data
	CPU::switch_to(reinterpret_cast<Thread*>(&_dummy_val), m_ap_idle);
}

unsigned Scheduler::pri_to_quants(uint8_t priority) {
	kassert(priority <= 140);

	if(priority < 120) {
		return (140 - priority) * 20;
	} else {
		return (140 - priority) * 5;
	}
}

void Scheduler::add_thread_to_rq(Thread* thread) {

	//  FIXME/SMP: Locking
	m_rq.add(thread);

}

