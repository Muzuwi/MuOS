#pragma once
#include <Arch/x86_64/PtraceRegs.hpp>
#include <SystemTypes.hpp>
#include <asm/unistd.h>

extern "C" void _ukernel_syscall_entry();

/*
 *  Definitions for all implemented syscalls
 *  DEFINE_SYSCALL(function_id, handler_ptr, argc, has_return_val)
 *  Function id's are taken directly from LibC
 */
#define SYSCALL_ENUMERATE \
	DEFINE_SYSCALL(__SYS_getpid, &Process::getpid, 0, true) \
	DEFINE_SYSCALL(__SYS_klog, &Process::klog, 1, false)    \
	DEFINE_SYSCALL(100, &Process::heap_alloc, 1, true)  \
	DEFINE_SYSCALL(254, &Thread::sys_msleep, 1, false)      \

namespace Syscall {
	void init();
	[[maybe_unused]] void syscall_handle(PtraceRegs* regs);
}