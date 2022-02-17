#pragma once
#include <LibGeneric/List.hpp>
#include <LibGeneric/Spinlock.hpp>
#include <Structs/KAtomic.hpp>
#include <SystemTypes.hpp>

class Thread;

class KSemaphore {
	gen::Spinlock m_lock;
	KAtomic<uint64> m_value;
	gen::List<Thread*> m_queue;
public:
	KSemaphore(uint64 initial_value = 0);
	~KSemaphore();

	void wait();
	void signal();
};
