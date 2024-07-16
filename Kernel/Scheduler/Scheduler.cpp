#include <Arch/x86_64/CPU.hpp>
#include <Core/Assert/Assert.hpp>
#include <Core/IRQ/InterruptDisabler.hpp>
#include <Core/MP/MP.hpp>
#include <LibFormat/Format.hpp>
#include <Process/Process.hpp>
#include <Process/Thread.hpp>
#include <Scheduler/Scheduler.hpp>
#include "Core/Log/Logger.hpp"
#include "LibGeneric/SharedPtr.hpp"
#include "LibGeneric/String.hpp"

void Scheduler::tick() {
	auto* thread = this_cpu()->current_thread();
	if(!thread) {
		return;
	}

	//  Cannot preempt, process is holding locks
	if(thread->preempt_count() > 0) {
		return;
	}

	if(thread->sched_ctx().quants_left > 0) {
		//  Spend one thread quantum
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
	thread->m_sched.priority = 19;
	return thread.get();
}

/*
 *  Creates the idle task for the current AP and enters it, kickstarting the scheduler on the current AP
 */
void Scheduler::bootstrap(Thread* ap_idle) {
	irq_local_disable();

	if(!ap_idle) {
		const auto node_id = this_cpu()->node_id;
		ap_idle = create_idle_task(node_id);
	}

	run_here(ap_idle);
	schedule_new();
}

unsigned Scheduler::pri_to_quants(uint8_t priority) {
	ENSURE(priority <= 140);

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
	core::irq::InterruptDisabler irq_disabler {};

	m_scheduler_lock.lock();
	m_rq.add_inactive(thread);
	m_scheduler_lock.unlock();

	thread->sched_ctx().quants_left = pri_to_quants(120 + thread->priority());
}

/*
 *  Called when a task voluntarily relinquishes its' CPU time
 */
void Scheduler::schedule() {
	core::irq::InterruptDisabler irq_disabler {};
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
	core::irq::InterruptDisabler irq_disabler {};
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
	core::irq::InterruptDisabler irq_disabler {};
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
		//  No active tasks are left; swap to the inactive queue
		m_rq.swap();
		next_thread = m_rq.find_runnable();
		//  There will ALWAYS be a runnable task in the queue (idle task is ALWAYS ready)
		//  If this isn't the case, something went terribly wrong
		if(!next_thread) {
			core::log::_push(core::log::LogLevel::Fatal, "scheduler", "BUG: Scheduler task queue empty!");
			ENSURE_NOT_REACHED();
		}
	}
	m_scheduler_lock.unlock();

	//  Detect the scenario when we're just bootstrapping a node
	//  In this case, the current thread will be nullptr
	//  Huge hack - use dummy buffer on the stack when switching for the first time,
	//  as we don't care about saving garbage data
	if(!thread) {
		uint8 _dummy_val[sizeof(Thread)] = {};
		CPU::switch_to(reinterpret_cast<Thread*>(&_dummy_val), next_thread);

		core::log::_push(core::log::LogLevel::Fatal, "scheduler",
		                 "BUG: Bootstrapping returned execution to the scheduler!");
		ENSURE_NOT_REACHED();
	}

	//  Only switch_to if an actual new thread needs to run
	//  If there is nothing else to run, we can potentially be resumed once again
	if(thread != next_thread) {
		CPU::switch_to(thread, next_thread);
	}
}

/*
 *  Wakes up a thread after block/sleep.
 */
void Scheduler::wake_up(Thread* thread) {
	if(!thread) {
		return;
	}

	core::irq::InterruptDisabler irq_disabler {};

	ENSURE(thread->state() != TaskState::Running);
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
	core::irq::InterruptDisabler irq_disabler {};

	m_scheduler_lock.lock();
	m_rq.dump_statistics();
	m_scheduler_lock.unlock();
}
