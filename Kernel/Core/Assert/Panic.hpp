#pragma once

namespace core {
	/** Trigger a kernel panic.
	 *
	 *  Prints some debugging information and halts execution of the kernel.
	 */
	[[noreturn]] void panic(char const*);

	/** Trigger a warning
	 *
	 *  Logs a warning to the kernel console, and prints some debugging information
	 *  to aid with debugging.
	 */
	void warn(char const*);
}
