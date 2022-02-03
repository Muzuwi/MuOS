#include <LibGeneric/Spinlock.hpp>
#include <LibGeneric/LockGuard.hpp>
#include <Process/PidAllocator.hpp>

using gen::Spinlock;
using gen::LockGuard;

static Spinlock s_pid_lock {};
static pid_t s_current_pid {3};

pid_t PidAllocator::next() {
	LockGuard<Spinlock> pid_lock {s_pid_lock};
	
	auto ret = s_current_pid;
	s_current_pid++;
	return ret;
}
