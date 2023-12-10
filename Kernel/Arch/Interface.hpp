#pragma once
#include <Core/Error/Error.hpp>

/** This file contains function definitions that must be implemented by
 *  all architectures.
 */

extern "C" {
	/**	Platform-specific boot entry function.
	 *
	 *  The architecture is responsible for saving its boot context and
	 *  jumping to core::start::start() to progress the kernel boot.
	 */
	[[maybe_unused]] [[noreturn]] void platform_boot_entry(void*);
}

namespace arch {
	/** Early platform initialization.
	 *  This should be used to initialize certain functionality early in the boot,
	 *  for example setting up early boot trap handlers or bare-bones serial output
	 *  for debugging.
	 */
	core::Error platform_early_init();

	/** Platform initialization
	 *  This should be used for creating kernel objects required to boot the kernel.
	 */
	core::Error platform_init();
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
}
