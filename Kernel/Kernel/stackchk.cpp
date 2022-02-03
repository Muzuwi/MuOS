#include <Debug/klogf.hpp>
#include <Kernel/Debug/kpanic.hpp>

unsigned long __stack_chk_guard;
//  FIXME: Would be great if the canary was randomized
extern "C" void __stack_chk_guard_setup(void) {
	__stack_chk_guard = 0xFABADADADABAFECA;
}

extern "C" void __stack_chk_fail(void) {
	kerrorf_static("[uKernel] Stack corruption detected, panic!\n");
	kpanic();
}
