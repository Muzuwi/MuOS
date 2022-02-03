#include <Debug/kdebugf.hpp>
#include <Arch/x86_64/TrapFrame.hpp>
#include <Process/Scheduler.hpp>
#include <Process/Process.hpp>
#include <Syscalls/SyscallList.hpp>

extern "C" uint32_t _ukernel_syscall_handler(TrapFrame regs) {
	const auto _struct = _SyscallParamPack(regs.ebx, regs.ecx, regs.edx, regs.esi, regs.edi);
	return Syscall::dispatch(regs.eax, _struct);
}
