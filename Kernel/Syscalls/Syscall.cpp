#include <Kernel/Debug/kdebugf.hpp>
#include <Kernel/Process/Process.hpp>
#include <Kernel/Syscalls/Syscall.hpp>

[[maybe_unused]] void Syscall::syscall_handle(PtraceRegs* regs) {
	kdebugf("Syscall %x%x in process pid=%i\n", regs->rax>>32u, regs->rax&0xffffffffu, Process::current()->pid());

//	while (true)
//		;
}
