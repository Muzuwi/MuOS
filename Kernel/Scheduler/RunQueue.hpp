#pragma once
#include <LibGeneric/List.hpp>

constexpr unsigned scheduler_priority_count() {
	return 140;
}

class Thread;

class RunQueue {
	gen::List<Thread*> m_queues[scheduler_priority_count()];
public:
	Thread* find_runnable() const;
	void add(Thread*);
	void remove(Thread*);
};
