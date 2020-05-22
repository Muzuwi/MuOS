#include <Kernel/Debug/kdebugf.hpp>
#include <Arch/i386/TrapFrame.hpp>
//#include <Arch/i386/Timer.hpp>
//#include <Kernel/Process/Scheduler.hpp>
//#include <Kernel/Process/Process.hpp>

extern "C" void _ukernel_syscall_handler(TrapFrame regs) {
//	kdebugf("[Syscall] Syscall, reg dump: \n");
//	kdebugf("eax: %x, ebx: %x, ecx: %x, edx: %x\n", regs.eax, regs.ebx, regs.ecx, regs.edx);
//	kdebugf("ebp: %x, esp: %x, esi: %x, edi: %x\n", regs.ebp, regs.esp, regs.esi, regs.edi);
//	kdebugf("eip: %x\n", regs.eip);

//	if(regs.eax == 0x01) {
//		kdebugf("[Syscall] Process %i wants to close!\n", Process::current()->pid());
//		Process::kill(Process::current()->pid());
//	}
}
