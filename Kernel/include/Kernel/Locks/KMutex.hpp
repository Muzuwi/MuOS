#pragma once
#include <LibGeneric/Spinlock.hpp>
#include <LibGeneric/List.hpp>

class Process;

struct KMutexWaiter {
	Process* m_waiter;
};

class KMutex {
	Process* m_owner;
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