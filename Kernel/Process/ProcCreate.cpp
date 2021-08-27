#include <Kernel/Process/Process.hpp>
#include <Kernel/Process/PidAllocator.hpp>
#include <Kernel/Memory/KHeap.hpp>

SharedPtr<Process> Process::create(ProcFlags flags) {
	return SharedPtr {
		new (KHeap::allocate(sizeof(Process))) Process(PidAllocator::next(), flags)
	};
}

