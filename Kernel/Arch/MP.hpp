#pragma once

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
}
