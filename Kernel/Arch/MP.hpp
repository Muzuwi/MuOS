#pragma once
#include <Core/Error/Error.hpp>

namespace core::task {
	struct Task;
}

namespace arch::mp {
	/** The environment contains kernel-specific information about the node that
	 * 	is currently executing kernel code.
	 *	The following requirements are placed on the platform w.r.t environment implementation:
	 * 	- Every node capable of executing code must have an independent environment
	 * 	- Calling environment_set sets the environment for the current node only
	 * 	- A call to environment_get following environment_set must return the same
	 * 	  value that was used in the call to environment_set.
	 * 	- Below must be available after platform_early_init returns
	 *  - Different nodes may access the environment pointers of other nodes
	 */

	/**	Sets the execution environment
	 */
	void environment_set(void*);

	/**	Gets the current execution environment
	 */
	void* environment_get();

	/**	Switches from task `prev` to a new task `next`.
	 *
	 *	The current execution context is saved into the task structure of `prev`, and
	 *	the execution of this task is paused. The execution of the task `next` is
	 * 	resumed at the point of the previous context switch.
	 */
	void switch_to(core::task::Task* prev, core::task::Task* next);

	/**	Switches execution to the specified task and kicks off the scheduler
	 *  on the current node.
	 *
	 *	While regular switch_to assumes that a task was already executing,
	 * 	switch_to_first is used instead when the scheduler loop is not yet
	 * 	running, and there is no task to save the context of.
	 */
	[[noreturn]] void switch_to_first(core::task::Task* task);

	/**	Prepare kernel task structure for execution.
	 *
	 * 	Prepare the task for kernel-mode execution with the specified entrypoint data. The
	 *	architecture should bootstrap the internal task state, so that a call to switch_to
	 * 	on the task results in the task being started.
	 */
	core::Error prepare_task(core::task::Task* task, void (*exec)(), void* stack, size_t stack_size);
}
