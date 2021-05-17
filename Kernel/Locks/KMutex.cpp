#include <Kernel/Locks/KMutex.hpp>
#include <Kernel/Scheduler/Scheduler.hpp>
#include <Kernel/Process/Process.hpp>

KMutex::KMutex() noexcept
: m_owner(nullptr), m_spinlock(), m_waiters() {
}

void KMutex::lock() {
	while(!_lock()) {
		//  Failed acquiring lock, add ourselves to the waiting list and reschedule
		wait();
	}
}

void KMutex::unlock() {
	_unlock();
}

bool KMutex::_lock() {
	//  Successfully acquired the lock
	if(m_spinlock.try_lock()) {
		m_owner = Process::current();
		return true;
	}

	return false;
}

bool KMutex::_unlock() {
	if(!m_owner)
		return false;

	if(m_owner && Process::current() != m_owner)
		return false;

	Process::current()->preempt_disable();
	m_owner = nullptr;
	m_spinlock.unlock();
	waiter_try_wake_up();
	Process::current()->preempt_enable();

	return true;
}

void KMutex::waiter_try_wake_up() {
	Process::current()->preempt_disable();

	m_waiters_lock.lock();
	if(!m_waiters.empty()) {
		auto mutex_waiter = m_waiters.front();
		m_waiters.pop_front();

		auto* process = mutex_waiter.m_waiter;
		assert(process->state() == ProcessState::Blocking);
		Scheduler::wake_up(process);
	}
	m_waiters_lock.unlock();

	Process::current()->preempt_enable();
}

void KMutex::wait() {
	Process::current()->preempt_disable();

	m_waiters_lock.lock();
	m_waiters.push_back(KMutexWaiter{.m_waiter = Process::current()});
 	m_waiters_lock.unlock();

 	//  FIXME_SMP: Race condition when different core would try waking up a process while we haven't been preempted
	Process::current()->set_state(ProcessState::Blocking);
	Scheduler::schedule();

	Process::current()->preempt_enable();
}
