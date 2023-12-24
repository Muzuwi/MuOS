#include <Core/Log/Logger.hpp>
#include <Core/MP/MP.hpp>
#include <Memory/Wrappers/UserPtr.hpp>
#include <Process/Process.hpp>
#include <Process/Thread.hpp>

CREATE_LOGGER("proc::syscall", core::log::LogLevel::Debug);

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
	log.debug("Thread[tid={}]: '{}'", Thread::current()->tid(), (char const*)kernel_str.get());
}

uint64 Process::heap_alloc(size_t region_size) {
	auto thread = this_cpu()->current_thread();
	auto retval = thread->parent()->vmm().allocate_user_heap(region_size);

	log.debug("Thread[tid={}]: heap_alloc={}", thread->tid(), Format::ptr(retval));
	return reinterpret_cast<uint64>(retval);
}
