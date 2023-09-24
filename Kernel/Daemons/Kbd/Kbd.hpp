#pragma once
#include <Arch/x86_64/Interrupt/IRQDispatcher.hpp>
#include <SystemTypes.hpp>

namespace Kbd {
	[[noreturn]] void kbd_thread();

	DEFINE_MICROTASK(kbd_microtask);
}
