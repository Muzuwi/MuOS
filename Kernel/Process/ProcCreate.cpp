#include <Process/Process.hpp>
#include <Process/PidAllocator.hpp>
#include <Memory/KHeap.hpp>

SharedPtr<Process> Process::create(ProcFlags flags) {
	return SharedPtr {
		new (KHeap::allocate(sizeof(Process))) Process(PidAllocator::next(), flags)
	};
}

