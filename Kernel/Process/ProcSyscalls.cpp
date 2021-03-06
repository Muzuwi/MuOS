#include <Kernel/Process/Process.hpp>
#include <Kernel/Memory/UserPtr.hpp>

pid_t Process::getpid() {
	return Process::current()->pid();
}

uint64_t Process::getpriority() {
	return Process::current()->priority();
}

/*
 *  FIXME: Simple debugging utility - should be removed
 */
void Process::klog(UserPtr<const char> str) {
	auto kernel_str = str.copy_to_kernel();
	if(!kernel_str)
		return;
	kdebugf("Process[pid=%i]: %s\n", Process::current()->pid(), kernel_str.get());
}
