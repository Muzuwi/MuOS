#pragma once
#include <Core/Task/Task.hpp>

namespace core::sched {
	/**	Maximum amount of nodes executing tasks that the scheduler
	 *	supports. This directly limits the amount of cores the kernel
	 *	can run on.
	 */
	static constexpr size_t SCHEDULER_MAX_NODE = 128;

	using BlockHandle = void*;

	/** Suspend the current task and switch to a different task
	 *
	 *  The current task voluntarily yields its CPU time and a
	 *  different task may be picked and switched to.
	 *
	 *  This function will return when execution is returned to
	 *  the current task.
	 *
	 * 	MP-Context: Current-Node
	 * 	IRQ-Safe: No
	 */
	void yield();

	/** Block the current task and switch to a different one
	 *
	 *  The current task is blocked and remains in State::Blocked
	 *  until explicitly unblocked by a different task. Until
	 *  it is unblocked, it will never be picked by the scheduler
	 *  for running.
	 *
	 *  This function will return when the task is unblocked and
	 *  execution is returned to it.
	 *
	 * 	MP-Context: Current-Node
	 * 	IRQ-Safe: No
	 */
	void block(BlockHandle&);

	/** Unblock a task
	 *
	 *  Unblocks a task that has previously called core::sched::block()
	 * 	with the given BlockHandle. It is safe to call this function
	 * 	from an interrupt context.
	 *
	 * 	MP-Context: Any-Node
	 * 	IRQ-Safe: Yes
	 */
	void unblock(BlockHandle&);

	/** Exit a task
	 *
	 *  Signal that the current task is going away and should be
	 *  removed entirely from the runlist.
	 *
	 * 	MP-Context: Current-Node
	 * 	IRQ-Safe: No
	 */
	[[noreturn]] void exit();

	/** Add a task to the scheduler
	 *
	 *  Informs the scheduler that a new task was created, which
	 *  should assign it to the runlist of the current node. This call
	 * 	may cause a context switch, if the new task is higher priority
	 *  than the current one.
	 *
	 * 	MP-Context: Current-Node
	 * 	IRQ-Safe: No
	 */
	void add_here(core::task::Task*);

	/**	Bootstrap execution of the scheduler
	 *
	 *	This is used to enter the scheduler for the first time on a given
	 *	node. Two tasks are required: the idle task and the init task. The
	 * 	scheduler will immediately start executing the init task once it is
	 * 	bootstrapped, which can be used for further init work on the current
	 * 	node.
	 *
	 * 	MP-Context: Current-Node
	 * 	IRQ-Safe: No
	 */
	[[noreturn]] void enter_scheduler_for_the_first_time(core::task::Task* idle, core::task::Task* init);
}
