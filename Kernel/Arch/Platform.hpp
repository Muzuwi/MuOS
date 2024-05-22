#pragma once
#include <Core/Error/Error.hpp>

extern "C" {
	/**	Platform-specific boot entry function.
	 *
	 *  The architecture is responsible for saving its boot context and
	 *  jumping to core::start::start() to progress the kernel boot.
	 */
	[[maybe_unused]] [[noreturn]] void platform_boot_entry(void*);

	/**	Platform-specific idle function.
	 *
	 * 	This is the function that runs in all idle tasks (on each CPU).
	 */
	[[maybe_unused]] [[noreturn]] void platform_idle();
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
