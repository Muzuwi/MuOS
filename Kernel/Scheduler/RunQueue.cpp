#include <LibGeneric/Algorithm.hpp>
#include <Kernel/Process/Process.hpp>
#include <Kernel/Scheduler/RunQueue.hpp>

RunnableList::RunnableList()
: m_lists(), m_usable(0) {
}

void RunnableList::add_process(Process* process) {
	m_lists[process->priority()].push_back(process);
	m_usable++;
}

void RunnableList::remove_process(Process* process) {
	auto it = gen::find(m_lists[process->priority()], process);
	m_lists[process->priority()].erase(it);
	if(it != m_lists->end())
		m_usable--;
}

size_t RunnableList::usable() const {
	return m_usable;
}
