#pragma once
#include <LibGeneric/List.hpp>

class Process;

constexpr unsigned scheduler_priority_count() {
	return 140;
}

class RunnableList {
	friend class Scheduler;

	gen::List<Process*> m_lists[scheduler_priority_count()];
	size_t m_usable;
public:
	RunnableList();
	void add_process(Process*);
	void remove_process(Process*);
	size_t usable() const;
};

struct RunQueue {
	RunnableList* m_active;
	RunnableList* m_expired;
	RunnableList m_first;
	RunnableList m_second;
	RunQueue() noexcept
	: m_active(&m_first), m_expired(&m_second), m_first(), m_second() {}
};