#pragma once
#include <Arch/i386/PtraceRegs.hpp>
#include <Kernel/SystemTypes.hpp>

extern "C" void _ukernel_syscall_entry();

namespace Syscall {
	[[maybe_unused]] void syscall_handle(PtraceRegs* regs);
}