#include <Debug/kassert.hpp>
#include <Locks/KMutex.hpp>
#include <Process/Thread.hpp>
#include <SMP/SMP.hpp>

KMutex::KMutex() noexcept
    : m_owner(nullptr)
    , m_spinlock()
    , m_waiters() {}

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
		m_owner = SMP::ctb().current_thread();
		return true;
	}

	return false;
}

bool KMutex::_unlock() {
	if(!m_owner)
		return false;

	auto thread = SMP::ctb().current_thread();
	if(thread != m_owner)
		return false;

	thread->preempt_disable();
	m_owner = nullptr;
	m_spinlock.unlock();
	waiter_try_wake_up();
	thread->preempt_enable();

	return true;
}

void KMutex::waiter_try_wake_up() {
	auto thread = SMP::ctb().current_thread();
	thread->preempt_disable();

	m_waiters_lock.lock();
	if(!m_waiters.empty()) {
		auto mutex_waiter = m_waiters.front();
		m_waiters.pop_front();

		auto* thread = mutex_waiter.m_waiter;
		kassert(thread->state() == TaskState::Blocking);
		SMP::ctb().scheduler().wake_up(thread);
	}
	m_waiters_lock.unlock();

	thread->preempt_enable();
}

void KMutex::wait() {
	auto thread = SMP::ctb().current_thread();
	thread->preempt_disable();

	m_waiters_lock.lock();
	m_waiters.push_back(KMutexWaiter { .m_waiter = thread });
	m_waiters_lock.unlock();

	//  FIXME_SMP: Race condition when different core would try waking up a process while we haven't been preempted
	SMP::ctb().scheduler().block();

	thread->preempt_enable();
}
