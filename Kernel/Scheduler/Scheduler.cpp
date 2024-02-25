#include <Arch/x86_64/CPU.hpp>
#include <Arch/x86_64/IRQDisabler.hpp>
#include <Core/MP/MP.hpp>
#include <Debug/kassert.hpp>
#include <LibFormat/Format.hpp>
#include <Process/Process.hpp>
#include <Process/Thread.hpp>
#include <Scheduler/Scheduler.hpp>
#include "Arch/Interface.hpp"
#include "LibGeneric/SharedPtr.hpp"
#include "LibGeneric/String.hpp"

void Scheduler::tick() {
	auto* thread = this_cpu()->current_thread();
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
	auto* current = this_cpu()->current_thread();
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
 *  Creates an idle task for the AP with the specified custom platform identifier
 */
Thread* Scheduler::create_idle_task(size_t identifier) {
	char s_buffer[64] {};
	Format::format("idle[{}]", s_buffer, sizeof(s_buffer), identifier);
	auto thread = Process::create_with_main_thread(gen::String { s_buffer }, Process::kerneld(), platform_idle);
	return thread.get();
}

/*
 *  Creates the idle task for the current AP and enters it, kickstarting the scheduler on the current AP
 */
void Scheduler::bootstrap(Thread* ap_idle) {
	CPU::irq_disable();

	if(!ap_idle) {
		const auto node_id = this_cpu()->node_id;
		ap_idle = create_idle_task(node_id);
	}
	m_ap_idle = ap_idle;

	//  Huge hack - use dummy buffer on the stack when switching for the first time,
	//  as we don't care about saving garbage data
	uint8 _dummy_val[sizeof(Thread)] = {};
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
	auto* thread = this_cpu()->current_thread();

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
	auto* thread = this_cpu()->current_thread();
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
	auto* thread = this_cpu()->current_thread();
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
	auto* thread = this_cpu()->current_thread();

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

	auto* current = this_cpu()->current_thread();
	if(thread->priority() <= current->priority()) {
		current->reschedule();
	}
}

/**
 * 	Run the specified thread within this scheduler
 *	The intention of this is to allow remote APs to run certain
 *	threads on the AP this scheduler is tied with.
 */
void Scheduler::run_here(Thread* thread) {
	if(!thread) {
		return;
	}
	add_thread_to_rq(thread);
}

void Scheduler::dump_statistics() {
	IRQDisabler irq_disabler {};

	m_scheduler_lock.lock();
	m_rq.dump_statistics();
	m_scheduler_lock.unlock();
}
