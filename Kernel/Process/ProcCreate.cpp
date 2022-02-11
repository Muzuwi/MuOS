#include <Process/Process.hpp>
#include <Process/PidAllocator.hpp>
#include <Memory/KHeap.hpp>

SharedPtr<Process> Process::create(gen::String name, ProcFlags flags) {
	return SharedPtr {
			new(KHeap::allocate(sizeof(Process))) Process(PidAllocator::next(), name, flags)
	};
}

SharedPtr<Thread> Process::create_with_main_thread(gen::String name, SharedPtr<Process> parent, void(* kernel_exec)(),
                                                   ProcFlags flags) {
	if(!parent) {
		return SharedPtr<Thread> { nullptr };
	}

	auto process = create(name, flags);
	if(!process) {
		return SharedPtr<Thread> { nullptr };
	}
	parent->add_child(process);
	kassert(process->vmm().clone_address_space_from(Process::kerneld()->vmm().pml4()));

	auto thread = Thread::create_in_process(process, kernel_exec);
	return thread;
}
