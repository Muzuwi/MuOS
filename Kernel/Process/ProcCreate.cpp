#include <Memory/KHeap.hpp>
#include <Process/PidAllocator.hpp>
#include <Process/Process.hpp>

SharedPtr<Process> Process::create(gen::String name, ProcFlags flags) {
	return SharedPtr { new(KHeap::instance().slab_alloc(sizeof(Process))) Process(PidAllocator::next(), name, flags) };
}

SharedPtr<Thread> Process::create_with_main_thread(gen::String name, SharedPtr<Process> parent, void (*kernel_exec)(),
                                                   ProcFlags flags) {
	if(!parent) {
		return SharedPtr<Thread> { nullptr };
	}

	auto process = create(gen::move(name), flags);
	if(!process) {
		return SharedPtr<Thread> { nullptr };
	}
	//  Clone base kernel mappings
	if(!process->vmm().clone_address_space_from(Process::kerneld()->vmm().pml4())) {
		return SharedPtr<Thread> { nullptr };
	}

	auto thread = Thread::create_in_process(process, kernel_exec);
	if(!thread) {
		return SharedPtr<Thread> { nullptr };
	}

	parent->add_child(process);

	return thread;
}
