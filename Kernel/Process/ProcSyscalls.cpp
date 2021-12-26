#include <Kernel/Process/Process.hpp>
#include <Kernel/Process/Thread.hpp>
#include <Kernel/Memory/UserPtr.hpp>
#include <Kernel/Memory/VMM.hpp>
#include <Kernel/SMP/SMP.hpp>

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
	kdebugf("Thread[tid=%i]: %s\n", Thread::current()->tid(), kernel_str.get());
}

uint64 Process::heap_alloc(size_t region_size) {
	auto thread = SMP::ctb().current_thread();
	auto retval = thread->parent()->vmm().allocate_user_heap(region_size);

	kdebugf("Thread[tid=%i]: heap_alloc=%x%x\n", thread->tid(), (uint64)retval>>32u, (uint64)retval&0xffffffffu);
	return reinterpret_cast<uint64>(retval);
}
