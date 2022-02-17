#include <Arch/x86_64/CPU.hpp>
#include <Arch/x86_64/IRQDisabler.hpp>
#include <Daemons/BootAP/BootAP.hpp>
#include <Daemons/Idle/Idle.hpp>
#include <Daemons/Kbd/Kbd.hpp>
#include <Daemons/SysDbg/SysDbg.hpp>
#include <Daemons/Testd/Testd.hpp>
#include <Debug/kassert.hpp>
#include <Debug/klogf.hpp>
#include <Interrupt/IRQDispatcher.hpp>
#include <Process/Process.hpp>
#include <Process/Thread.hpp>
#include <Scheduler/Scheduler.hpp>
#include <SMP/SMP.hpp>

void Scheduler::tick() {
	auto* thread = SMP::ctb().current_thread();
	if(!thread) {
		return;
	}

	if(thread == m_ap_idle) {
		m_scheduler_lock.lock();

		//  Force reschedule when at least one thread becomes runnable
		if(m_rq.find_runnable() != nullptr) {
			thread->reschedule();
		} else {
			m_rq.swap();
			thread->clear_reschedule();
		}

		m_scheduler_lock.unlock();
		return;
	}

	//  Cannot preempt, process is holding locks
	if(thread->preempt_count() > 0) {
		return;
	}

	//  Spend one thread quantum
	if(thread->sched_ctx().quants_left > 0) {
		thread->sched_ctx().quants_left--;
	} else {
		//  Reschedule currently running thread - ran out of quants
		thread->reschedule();
		thread->sched_ctx().quants_left = pri_to_quants(120 + thread->priority());
	}
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

/*
 *  Creates an idle task for the AP with the specified APIC ID
 */
Thread* Scheduler::create_idle_task(uint8 apic_id) {
	static char s_buffer[64] {};
	Format::format("Idled[{}]", s_buffer, 256, apic_id);
	auto thread = Process::create_with_main_thread({ s_buffer }, Process::kerneld(), Idle::idle_thread);
	return thread.get();
}

/*
 *  Creates the idle task for the current AP and enters it, kickstarting the scheduler on the current AP
 */
void Scheduler::bootstrap() {
	CPU::irq_disable();
	klogf("[Scheduler] Bootstrapping AP {}\n", SMP::ctb().current_ap());

	m_ap_idle = create_idle_task(SMP::ctb().current_ap());
	//  Update permanent process names
	//  These process structs are actually contained within the kernel executable (but wrapped in a SharedPtr that
	//  should hopefully never be deallocated). Because strings cause allocations, initializing them inline is
	//  impossible
	Process::init()->m_simple_name = "Init";
	Process::kerneld()->m_simple_name = "Kerneld";

	{
		auto new_process = Process::init();
		kassert(new_process->vmm().clone_address_space_from(Process::kerneld()->vmm().pml4()));

		auto new_thread = Thread::create_in_process(new_process, Testd::userland_test_thread);
		new_thread->sched_ctx().priority = 10;
		add_thread_to_rq(new_thread.get());
	}

	{
		auto new_thread = Process::create_with_main_thread("Testd", Process::kerneld(), Testd::test_kernel_thread);
		add_thread_to_rq(new_thread.get());
	}

	{
		auto keyboard_thread = Process::create_with_main_thread("Kbd", Process::kerneld(), Kbd::kbd_thread);
		keyboard_thread->sched_ctx().priority = 0;
		add_thread_to_rq(keyboard_thread.get());
		IRQDispatcher::register_microtask(1, Kbd::kbd_microtask);
	}

	{
		auto ap_start_thread = Process::create_with_main_thread("BootAP", Process::kerneld(), BootAP::boot_ap_thread);
		add_thread_to_rq(ap_start_thread.get());
	}

	{
		auto debugger_thread = Process::create_with_main_thread("SysDbg", Process::kerneld(), SysDbg::sysdbg_thread);
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

/*
 *  Adds a thread to be run to the inactive thread queue.
 *  The thread will be run after all threads in the active queue are
 *  preempted/block/sleep.
 */
void Scheduler::add_thread_to_rq(Thread* thread) {
	IRQDisabler irq_disabler {};

	m_scheduler_lock.lock();
	m_rq.add_inactive(thread);
	m_scheduler_lock.unlock();

	thread->sched_ctx().quants_left = pri_to_quants(120 + thread->priority());
}

/*
 *  Called when a task voluntarily relinquishes its' CPU time
 */
void Scheduler::schedule() {
	IRQDisabler irq_disabler {};
	auto* thread = SMP::ctb().current_thread();

	m_scheduler_lock.lock();
	m_rq.remove_active(thread);
	m_rq.add_inactive(thread);
	m_scheduler_lock.unlock();

	schedule_new();
}

/*
 *  Called when a thread wants to sleep.
 *  The thread is not re-added to the inactive queue.
 */
void Scheduler::sleep() {
	IRQDisabler irq_disabler {};
	auto* thread = SMP::ctb().current_thread();
	thread->set_state(TaskState::Sleeping);

	m_scheduler_lock.lock();
	m_rq.remove_active(thread);
	m_scheduler_lock.unlock();

	schedule_new();
}

/*
 *  Called when a thread is blocking on I/O.
 *  The thread is not re-added to the inactive queue.
 */
void Scheduler::block() {
	IRQDisabler irq_disabler {};
	auto* thread = SMP::ctb().current_thread();
	thread->set_state(TaskState::Blocking);

	m_scheduler_lock.lock();
	m_rq.remove_active(thread);
	m_scheduler_lock.unlock();

	schedule_new();
}

/*
 *  Schedules a new task to run and switches to it
 */
void Scheduler::schedule_new() {
	auto* thread = SMP::ctb().current_thread();

	//  Find next runnable task
	m_scheduler_lock.lock();
	auto* next_thread = m_rq.find_runnable();
	if(!next_thread) {
		m_rq.swap();
		next_thread = m_ap_idle;
	}
	m_scheduler_lock.unlock();

	CPU::switch_to(thread, next_thread);
}

/*
 *  Wakes up a thread after block/sleep.
 */
void Scheduler::wake_up(Thread* thread) {
	if(!thread) {
		return;
	}

	IRQDisabler irq_disabler {};

	kassert(thread->state() != TaskState::Running);
	thread->set_state(TaskState::Ready);

	m_scheduler_lock.lock();
	m_rq.add_inactive(thread);
	m_scheduler_lock.unlock();

	auto* current = SMP::ctb().current_thread();
	if(thread->priority() <= current->priority()) {
		current->reschedule();
	}
}

void Scheduler::dump_statistics() {
	//	IRQDisabler irq_disabler {};

	//	m_scheduler_lock.lock();
	m_rq.dump_statistics();
	//	m_scheduler_lock.unlock();
}
