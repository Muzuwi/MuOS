#pragma once
#include "SystemTypes.hpp"
#include "Interrupt/IRQDispatcher.hpp"

namespace Kbd {
	[[noreturn]] void kbd_thread();

	DEFINE_MICROTASK(kbd_microtask);
}
