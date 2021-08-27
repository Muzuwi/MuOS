#include <Kernel/Process/Process.hpp>
#include <Kernel/Process/Thread.hpp>
#include <Kernel/Memory/UserPtr.hpp>

pid_t Process::getpid() {
	return Thread::current()->parent()->pid();
}

/*
 *  FIXME: Simple debugging utility - should be removed
 */
void Process::klog(UserPtr<const char> str) {
	auto kernel_str = str.copy_to_kernel();
	if(!kernel_str)
		return;
	kdebugf("Thread[tid=%i]: %s\n", Thread::current()->tid(), kernel_str.get());
}
