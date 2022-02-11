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
	process->vmm().m_pml4 = process->vmm().clone_pml4(Process::kerneld()->vmm().pml4()).unwrap();

	auto thread = Thread::create_in_process(process, kernel_exec);
	return thread;
}
