#include <string.h>
#include <Arch/i386/GDT.hpp>
#include <Kernel/Process/PidAllocator.hpp>
#include <Kernel/Process/Process.hpp>
#include <LibGeneric/Mutex.hpp>
#include <LibGeneric/LockGuard.hpp>

Process* Process::create(ProcessFlags flags, KOptional<pid_t> force_pid) {
	if(force_pid.has_value())
		return new (KHeap::allocate(sizeof(Process))) Process(force_pid.unwrap(), flags);
	else
		return new (KHeap::allocate(sizeof(Process))) Process(PidAllocator::next(), flags);
}

/*
 *  Creates a Process struct for the idle task, and prepares a bootstrap stack frame
 */
Process* Process::create_idle_task(void(*function)()) {
	auto* kernel_idle = Process::create({.kernel_thread=1}, {0});
	gen::LockGuard<Mutex> lock {kernel_idle->m_process_lock};

	kernel_idle->m_pml4 = VMM::kernel_pml4();
	kernel_idle->m_kernel_stack_bottom = kernel_idle->create_kernel_stack();
	kernel_idle->m_interrupted_task_frame = (InactiveTaskFrame*)kernel_idle->modify_kernel_stack_bootstrap((void*)function);
	kernel_idle->m_state = ProcessState::Ready;

	return kernel_idle;
}


Process* Process::create_kernel_thread(void (*function)()) {
	auto* kthread = Process::create({.kernel_thread=1});
	gen::LockGuard<Mutex> lock {kthread->m_process_lock};

	kthread->m_pml4 = VMM::kernel_pml4()->clone(kthread->m_address_space);
	kthread->m_kernel_stack_bottom = kthread->create_kernel_stack();
	kthread->m_interrupted_task_frame = (InactiveTaskFrame*)kthread->modify_kernel_stack_bootstrap((void*)function);
	kthread->m_state = ProcessState::Ready;
	kthread->m_priority = 0;

	return kthread;
}

extern "C" void _task_enter_bootstrap();


/*
 *  Modifies the kernel stack such that it can be used to enter the task for the first time
 */
void* Process::modify_kernel_stack_bootstrap(void* ret_address) {
	auto vmregion = m_address_space.find_vmregion((uint8_t*)m_kernel_stack_bottom-1);
	//  FIXME:  Terrible hack, we have to access the stack page via PhysPtr's, because it is not yet mapped
	//  Possibly add a constructor to VMapping that allows creating it with preallocated pages
	auto* kernel_stack = vmregion.unwrap()->mapping().pages().back().end().as<uint8_t>().get_mapped();

	auto struct_begin = kernel_stack - sizeof(PtraceRegs);

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

	//  Return address (in the ***task***)
	regs->rip = (uint64_t)ret_address;

	//  Set rflags to known good value
	regs->rflags = 0x0200;

	regs->cs = (m_flags.kernel_thread ? GDT::get_kernel_CS() : GDT::get_user_CS() | 3);
	regs->ss = (m_flags.kernel_thread ? GDT::get_kernel_DS() : GDT::get_user_DS() | 3);

	//  Set up user stack
	if(!m_flags.kernel_thread) {
		auto stack_mapping = VMapping::create((void*)0x7fff00000000, 0x4000, VM_READ | VM_WRITE, MAP_PRIVATE);
		stack_mapping->map(this);
		m_address_space.create_vmregion(gen::move(stack_mapping));

		regs->rsp = (0x7fff00000000 + 0x4000);
	} else {
		regs->rbp = (uint64_t)m_kernel_stack_bottom;
		regs->rsp = (uint64_t)m_kernel_stack_bottom;
	}

	auto* kernel_return_addr = struct_begin - sizeof(uint64_t);
	*((uint64_t*)kernel_return_addr) = (uint64_t)_task_enter_bootstrap;

	auto* inactive_struct = kernel_return_addr - sizeof(InactiveTaskFrame);
	memset(inactive_struct, 0x0, sizeof(InactiveTaskFrame));

	auto* vm_kernel_stack = (uint8_t*)m_kernel_stack_bottom - sizeof(PtraceRegs) - sizeof(uint64_t) - sizeof(InactiveTaskFrame);

	return vm_kernel_stack;
}


Process* Process::create_userland_test() {
	auto* process = Process::create({});

	s_current = process;
	process->m_pml4 = VMM::kernel_pml4()->clone(process->m_address_space);
	process->m_kernel_stack_bottom = process->create_kernel_stack();
	process->m_interrupted_task_frame = (InactiveTaskFrame*)process->modify_kernel_stack_bootstrap((void*)0x100000);
	process->m_priority = 0;
	process->m_quants_left = 0;

	auto mapping = VMapping::create((void*)0x100000, 0x1000, VM_READ | VM_WRITE | VM_EXEC, MAP_SHARED);
	mapping->map(process);
	process->m_address_space.create_vmregion(gen::move(mapping));
	auto page = mapping->pages().front().base().as<uint8_t>();
	uint8_t bytes[] = { 0xb8, 0x10, 0x0, 0x10, 0x0, 0xc7, 0x0, 0x0, 0x0, 0x0, 0x0, 0x90, 0xeb, 0xfd };
	for(auto& b : bytes) {
		*page = b;
		page++;
	}
	process->m_state = ProcessState::Ready;
	s_current = nullptr;

	return process;
}


/*
 *  Completes the task switch process
 *  After returning from this function, execution resumes in the next task
 *  At this point, we're running on the next task's stack, in a non-preemptible context
 */
[[maybe_unused]]
void Process::finalize_switch(Process* prev, Process* next) {
	if(prev->state() == ProcessState::Running)
		prev->set_state(ProcessState::Ready);

	s_current = next;
	s_current->set_state(ProcessState::Running);

	//  Reset IRQ stack in the TSS
	GDT::set_irq_stack(next->m_kernel_stack_bottom);
}


/*
 *  Allocates a kernel stack to the currently set process
 */
void* Process::create_kernel_stack() {
	//  TODO:  Actually randomize
	auto random = [](size_t, size_t) -> size_t {
		return 4;
	};

	auto kstack_top = (uintptr_t)&_ukernel_virt_kstack_start
	                  + VMM::kernel_stack_size() * random(0, 0x3fffe00);
	auto kstack_bottom = kstack_top + VMM::kernel_stack_size();

	auto mapping = VMapping::create((void*)kstack_top, VMM::kernel_stack_size(), VM_READ | VM_WRITE | VM_KERNEL, MAP_PRIVATE);
	mapping->map(this);
	m_address_space.create_vmregion(gen::move(mapping));

	return (void*)kstack_bottom;
}