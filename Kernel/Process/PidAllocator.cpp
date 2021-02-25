#include <LibGeneric/Mutex.hpp>
#include <LibGeneric/LockGuard.hpp>
#include <Kernel/Process/PidAllocator.hpp>

using gen::Mutex;
using gen::LockGuard;

static Mutex s_pid_lock {};
static pid_t s_current_pid {1};

pid_t PidAllocator::next() {
	LockGuard<Mutex> pid_lock {s_pid_lock};
	
	auto ret = s_current_pid;
	s_current_pid++;
	return ret;
}
