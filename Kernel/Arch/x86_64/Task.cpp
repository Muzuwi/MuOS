#include <Arch/MP.hpp>
#include <Arch/x86_64/CPU.hpp>
#include <Arch/x86_64/GDT.hpp>
#include <Arch/x86_64/InactiveTaskFrame.hpp>
#include <Arch/x86_64/MP/ExecutionEnvironment.hpp>
#include <Arch/x86_64/PtraceRegs.hpp>
#include <Core/Assert/Assert.hpp>
#include <Core/Error/Error.hpp>
#include <Core/Mem/VM.hpp>
#include <Core/Task/Task.hpp>
#include <string.h>
#include <SystemTypes.hpp>

ASM_LINKAGE void _switch_to_asm(core::task::Task*, core::task::Task*);
ASM_LINKAGE void _task_enter_bootstrap();

ASM_LINKAGE void task_finalize_switch(core::task::Task* prev, core::task::Task* next) {
	if(prev->state == core::task::State::Running) {
		prev->state = core::task::State::Ready;
	}

	//  Save previous process' kernel GS base (userland GSbase when task is a ring3 task,
	//  and unused GSbase for kernel threads)
	prev->kernel_gs_base = (void*)CPU::get_kernel_gs_base();

	//  Set new process as current in CTB
	this_cpu()->set_task(next);

	//  x86_64: Reset IRQ stack in the TSS
	this_execution_environment()->gdt.set_irq_stack(next->kernel_stack_bottom);
	//  x86_64: Set the thread pointer in ExecutionEnvironment
	this_execution_environment()->task = next;

	//  Restore saved kernel gs base of next process
	CPU::set_kernel_gs_base((void*)next->kernel_gs_base);

	next->state = core::task::State::Running;
}

void arch::mp::switch_to(core::task::Task* prev, core::task::Task* next) {
	_switch_to_asm(prev, next);
}

static void* _bootstrap_task_stack(uint8* kernel_stack_bottom, PtraceRegs state) {
	auto struct_begin = kernel_stack_bottom - sizeof(PtraceRegs);

	auto* regs = reinterpret_cast<PtraceRegs*>(struct_begin);
	memset(regs, 0x0, sizeof(PtraceRegs));

	//  We're modifying the kernel stack as follows:
	//  ----  stack bottom  ----
	//  <ptrace regs>
	//  <ret rip pointing to _task_enter_bootstrap>
	//  rbp
	//  rbx
	//  r12
	//  r13
	//  r14
	//  r15   <--- rsp

	*regs = state;

	auto* kernel_return_addr = struct_begin - sizeof(uint64_t);
	*((uint64_t*)kernel_return_addr) = (uint64_t)&_task_enter_bootstrap;

	auto* inactive_struct = kernel_return_addr - sizeof(InactiveTaskFrame);
	memset(inactive_struct, 0x0, sizeof(InactiveTaskFrame));

	auto offset = (uint8*)kernel_stack_bottom - sizeof(PtraceRegs) - sizeof(uint64_t) - sizeof(InactiveTaskFrame);
	return offset;
}

core::Error arch::mp::prepare_task(core::task::Task* task, void (*exec)(), void* stack_top, size_t stack_size) {
	if(!task || !stack_top || !stack_size || !exec) {
		return core::Error::InvalidArgument;
	}

	auto* stack_bottom = reinterpret_cast<uint8*>(stack_top) + stack_size;
	PtraceRegs state = PtraceRegs::kernel_default();
	state.rip = (uint64)exec;
	state.rsp = (uint64)stack_bottom;
	state.rbp = (uint64)stack_bottom;

	task->kernel_stack_bottom = stack_top;
	task->interrupted_task_frame = _bootstrap_task_stack(stack_bottom, state);
	task->pml4 = core::mem::get_vmroot();

	return core::Error::Ok;
}
