#include <LibGeneric/Algorithm.hpp>
#include <Kernel/Process/Process.hpp>
#include <Kernel/Scheduler/RunQueue.hpp>

Thread* RunQueue::find_runnable() const {
	for(auto& list : m_queues) {
		if(list.empty()) continue;
		for(auto* thread : list) {
			if(thread->state() == TaskState::Ready)
				return thread;
		}
	}

	return nullptr;
}

void RunQueue::add(Thread* thread) {
	auto pri = thread->sched_ctx().priority;
	if(pri >= 140) pri = 139;

	m_queues[pri].push_back(thread);
}

void RunQueue::remove(Thread*) {

}
