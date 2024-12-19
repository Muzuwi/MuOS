#include <Arch/MP.hpp>
#include <Core/Assert/Assert.hpp>
#include <Core/Assert/Panic.hpp>
#include <Core/Error/Error.hpp>
#include <Core/IRQ/InterruptDisabler.hpp>
#include <Core/MP/MP.hpp>
#include <Core/Task/AvlRunList.hpp>
#include <Core/Task/Scheduler.hpp>
#include <Core/Task/SchedulerPlatform.hpp>
#include <Core/Task/Task.hpp>
#include <Structs/KAtomic.hpp>
#include "LibGeneric/Algorithm.hpp"

struct NodeRunList {
public:
	core::sched::AvlRunList* active;
	core::sched::AvlRunList* inactive;
	core::task::Task* idle {};

	constexpr NodeRunList()
	    : active(&first)
	    , inactive(&second) {};

	constexpr void swap() { gen::swap(active, inactive); }
private:
	core::sched::AvlRunList first;
	core::sched::AvlRunList second;
};

static NodeRunList s_runlists[core::sched::SCHEDULER_MAX_NODE] {};

static NodeRunList* sched_get_runlist(core::task::NodeId node) {
	if(node >= core::sched::SCHEDULER_MAX_NODE) {
		return nullptr;
	}
	return &s_runlists[node];
}

static void sched_runlist_add(core::task::Task* task) {
	if(!task) [[unlikely]] {
		return;
	}
	auto* rlist = sched_get_runlist(task->node_id);
	if(!rlist) [[unlikely]] {
		core::panic("No runlist available for the current node");
	}
	const auto err = rlist->active->add(task);
	if(err != core::Error::Ok) {
		core::panic("Adding task to runlist failed");
	}
}

static void sched_runlist_remove(core::task::Task* task) {
	if(!task) [[unlikely]] {
		return;
	}
	auto* rlist = sched_get_runlist(task->node_id);
	if(!rlist) [[unlikely]] {
		core::panic("No runlist available for the current node");
	}
	const auto err = rlist->active->remove(task);
	if(err != core::Error::Ok) [[unlikely]] {
		core::panic("Removing task from runlist failed");
	}
}

static void sched_runlist_move_to_inactive(core::task::Task* task) {
	if(!task) [[unlikely]] {
		return;
	}
	auto* rlist = sched_get_runlist(task->node_id);
	if(!rlist) [[unlikely]] {
		core::panic("No runlist available for the current node");
	}
	//  Yielding from the idle task doesn't need any special handling
	if(task == rlist->idle) {
		return;
	}
	//  When yielding a different task, we need to move it from active
	//  to the inactive runlist.
	if(const auto err = rlist->active->remove(task); err != core::Error::Ok) [[unlikely]] {
		core::panic("Move to inactive: removing task from active runlist failed");
	}
	if(const auto err = rlist->inactive->add(task); err != core::Error::Ok) [[unlikely]] {
		core::panic("Move to inactive: adding task to inactive runlist failed");
	}
}

static void sched_runlist_set_idle(core::task::Task* idle) {
	if(!idle) [[unlikely]] {
		core::panic("Cannot set null task as idle");
	}
	auto* rlist = sched_get_runlist(idle->node_id);
	if(!rlist) [[unlikely]] {
		core::panic("No runlist available for the current node");
	}
	rlist->idle = idle;
}

core::task::Task* sched_pick_next(core::task::NodeId node) {
	auto* rlist = sched_get_runlist(node);
	if(!rlist) [[unlikely]] {
		return nullptr;
	}
	//  If the active runlist does not have any more tasks, swap the runlists.
	auto* next = rlist->active->next();
	if(!next) {
		rlist->swap();
		next = rlist->active->next();
	}
	return next;
}

static void sched_new() {
	auto* current = this_cpu()->current_task();
	auto* next = sched_pick_next(this_cpu()->node_id);

	//  If there is no runnable tasks, run the idle task instead
	if(!next) {
		auto* runlist = sched_get_runlist(this_cpu()->node_id);
		next = runlist->idle;
	}
	//  Recalculate the amount of quants the task can run for
	next->quants_left = core::task::priority_to_quants(next->priority);
	//  We can only switch contexts when a different task needs to run
	//  Otherwise, just return to the currently running taks
	if(current != next) {
		arch::mp::switch_to(current, next);
	}
}

static core::sched::BlockHandle sched_acquire_block_handle(core::task::Task* task) {
	return task;
}

static core::task::Task* sched_block_handle_to_task(core::sched::BlockHandle handle) {
	return reinterpret_cast<core::task::Task*>(handle);
}

void core::sched::tick() {
	auto* task = this_cpu()->current_task();
	if(!task) {
		return;
	}

	//  Cannot preempt, task is holding locks
	if(task->preempt_count.load(MemoryOrdering::SeqCst) > 0) {
		return;
	}

	if(task->quants_left > 0) {
		//  Spend one task quantum
		task->quants_left--;
	} else {
		//  Reschedule currently running task - ran out of quants
		task->need_resched = true;
	}
}

void core::sched::on_interrupt_exit() {
	auto* task = this_cpu()->current_task();
	if(!task) {
		return;
	}
	if(!task->need_resched) {
		return;
	}
	task->need_resched = false;
	core::sched::yield();
}

void core::sched::yield() {
	auto* task = this_cpu()->current_task();
	if(!task) {
		return;
	}

	core::irq::InterruptDisabler irq_disabler {};
	task->state = task::State::Ready;
	sched_runlist_move_to_inactive(task);
	//  Reschedule the current task, but it may be resumed once no other
	//  tasks are runnable.
	sched_new();
}

void core::sched::block(BlockHandle& handle) {
	auto* task = this_cpu()->current_task();
	if(!task) {
		return;
	}

	core::irq::InterruptDisabler irq_disabler {};
	task->state = task::State::Blocked;
	sched_runlist_remove(task);

	//  Store the handle in caller-provided buffer
	handle = sched_acquire_block_handle(task);
	sched_new();
	//  Erase the handle to prevent double-unblock issues
	handle = nullptr;
}

void core::sched::unblock(BlockHandle& handle) {
	auto* task = sched_block_handle_to_task(handle);
	if(!task) {
		return;
	}

	core::irq::InterruptDisabler irq_disabler {};
	if(task->node_id == this_cpu()->node_id) {
		//  Simple case: unblocking a task running on the current node

		task->state = task::State::Ready;
		sched_runlist_add(task);
		//  An unblock may cause a higher priority task to be unblocked. We cannot
		//  immediately switch to it, as we could be running in an interrupt context.
		//  Trigger a reschedule of the current task.
		auto* current = this_cpu()->current_task();
		if(task::is_higher_priority(task->priority, current->priority)) {
			current->need_resched = true;
		}
	} else {
		//  Complex case: unblocking a task that is running on a different node
		//  Currently unimplemented, as it requires IPIs.
		core::panic("Remote scheduling is currently unsupported!");
	}
}

[[noreturn]] void core::sched::exit() {
	auto* task = this_cpu()->current_task();
	if(!task) {
		core::panic("Call to core::sched::exit with no task running!");
	}

	core::irq::InterruptDisabler irq_disabler {};
	//  Mark the task as no longer runnable and remove it from the current runlist permanently.
	task->state = task::State::Leaving;
	sched_runlist_remove(task);
	//  Switch to the next runnable task. This will never return, as the task can never be
	//  re-added to the runlist.
	sched_new();

	//  Unreachable, the task can never be scheduled afterwards
	HANG();
}

void core::sched::add_here(core::task::Task* task) {
	if(!task) {
		return;
	}

	core::irq::InterruptDisabler irq_disabler {};
	//  Assign the task to the current node
	task->node_id = this_cpu()->node_id;
	sched_runlist_add(task);
	//  A new task may have a higher priority than the current one. Defer the
	//  switch to the next scheduler tick.
	auto* current = this_cpu()->current_task();
	if(task::is_higher_priority(task->priority, current->priority)) {
		current->need_resched = true;
	}
}

void core::sched::enter_scheduler_for_the_first_time(core::task::Task* idle, core::task::Task* start) {
	if(!idle) {
		core::panic("Call to enter_scheduler_for_the_first_time with null idle task!");
	}
	if(!start) {
		core::panic("Call to enter_scheduler_for_the_first_time with null tart task!");
	}

	core::irq::InterruptDisabler irq_disabler {};

	//  Assign the start task to the current node
	start->node_id = this_cpu()->node_id;
	sched_runlist_add(start);

	//  Assign the idle task to the current node
	idle->node_id = this_cpu()->node_id;
	//  Idle task must be the lowest priority available, so it can be preempted
	//  at any time.
	idle->priority = task::_TaskPriorityIdle;
	sched_runlist_set_idle(idle);

	//  Switch to the start task, which will kick off the scheduler loop
	core::task::Task dummy_task;//  Don't care about preserving old state
	arch::mp::switch_to(&dummy_task, start);

	//  Unreachable, the scheduler is now running
	HANG();
}
