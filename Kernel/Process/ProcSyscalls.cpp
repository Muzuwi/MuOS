#include <Process/Process.hpp>
#include <Process/Thread.hpp>
#include <Memory/UserPtr.hpp>
#include <Memory/VMM.hpp>
#include <SMP/SMP.hpp>
#include <Debug/klogf.hpp>

pid_t Process::getpid() {
	return Thread::current()->parent()->pid();
}

/*
 *  FIXME: Simple debugging utility - should be removed
 */
void Process::klog(UserString str) {
	auto kernel_str = str.copy_to_kernel();
	if(!kernel_str)
		return;
	klogf("Thread[tid={}]: '{}'\n", Thread::current()->tid(), (char const*)kernel_str.get());
}

uint64 Process::heap_alloc(size_t region_size) {
	auto thread = SMP::ctb().current_thread();
	auto retval = thread->parent()->vmm().allocate_user_heap(region_size);

	klogf("Thread[tid={}]: heap_alloc={}\n", thread->tid(), Format::ptr(retval));
	return reinterpret_cast<uint64>(retval);
}
