#pragma once
#include <LibGeneric/PriorityQueue.hpp>

class Thread;

class RunQueue {
	struct PriorityComparator {
		bool operator()(Thread* const& lhs, Thread* const& rhs);
	};

	gen::PriorityQueue<Thread*, PriorityComparator> m_first;
	gen::PriorityQueue<Thread*, PriorityComparator> m_second;

	gen::PriorityQueue<Thread*, PriorityComparator>* m_active;
	gen::PriorityQueue<Thread*, PriorityComparator>* m_inactive;
public:
	RunQueue();

	Thread* find_runnable() const;
	void add_inactive(Thread*);
	void remove_active(Thread*);
	void swap();
	void dump_statistics();
};
