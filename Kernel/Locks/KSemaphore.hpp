#pragma once
#include <Kernel/SystemTypes.hpp>
#include <Kernel/Structs/KAtomic.hpp>
#include <LibGeneric/Spinlock.hpp>
#include <LibGeneric/List.hpp>

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
