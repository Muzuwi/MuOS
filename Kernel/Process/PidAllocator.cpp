#include <LibGeneric/LockGuard.hpp>
#include <LibGeneric/Spinlock.hpp>
#include <Process/PidAllocator.hpp>

using gen::LockGuard;
using gen::Spinlock;

static Spinlock s_pid_lock {};
static pid_t s_current_pid { 3 };

pid_t PidAllocator::next() {
	LockGuard<Spinlock> pid_lock { s_pid_lock };

	auto ret = s_current_pid;
	s_current_pid++;
	return ret;
}
