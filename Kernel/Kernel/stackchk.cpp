#include <Core/Log/Logger.hpp>
#include <Debug/kpanic.hpp>

CREATE_LOGGER("assert", core::log::LogLevel::Debug);

unsigned long __stack_chk_guard;
//  FIXME: Would be great if the canary was randomized
extern "C" void __stack_chk_guard_setup(void) {
	__stack_chk_guard = 0xFABADADADABAFECA;
}

extern "C" void __stack_chk_fail(void) {
	log.fatal("Stack corruption detected!");
	kpanic();
}
