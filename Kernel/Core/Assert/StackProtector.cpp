#include <Core/Assert/Panic.hpp>
#include <Core/Log/Logger.hpp>

unsigned long __stack_chk_guard;

extern "C" void __stack_chk_guard_setup(void) {
	//  TODO: Randomize the stack canary
	__stack_chk_guard = 0x5c38c91558667782;
}

extern "C" void __stack_chk_fail(void) {
	core::panic("Stack corruption detected");
}
