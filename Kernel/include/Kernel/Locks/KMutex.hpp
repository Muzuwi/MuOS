#pragma once
#include <LibGeneric/Spinlock.hpp>
#include <LibGeneric/List.hpp>

class Thread;

struct KMutexWaiter {
	Thread* m_waiter;
};

class KMutex {
	Thread* m_owner;
	gen::Spinlock m_spinlock;
	gen::List<KMutexWaiter> m_waiters;
	gen::Spinlock m_waiters_lock;

	bool _lock();
	bool _unlock();
	void waiter_try_wake_up();
	void wait();
public:
	KMutex() noexcept;
	void lock();
	void unlock();
};