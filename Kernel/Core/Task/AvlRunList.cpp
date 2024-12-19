#include <Core/Error/Error.hpp>
#include <Core/Task/AvlRunList.hpp>
#include <Core/Task/Task.hpp>
#include <LibGeneric/AVL.hpp>
#include <LibGeneric/LockGuard.hpp>

core::Error core::sched::AvlRunList::add(core::task::Task* task) {
	gen::LockGuard lg { m_lock };
	if(!task) {
		return Error::InvalidArgument;
	}
	return m_tree.add(*task) ? Error::Ok : Error::InvalidArgument;
}

core::Error core::sched::AvlRunList::remove(core::task::Task* task) {
	gen::LockGuard lg { m_lock };
	if(!task) {
		return Error::InvalidArgument;
	}
	return m_tree.remove(*task) ? Error::Ok : Error::InvalidArgument;
}

core::task::Task* core::sched::AvlRunList::next() {
	gen::LockGuard lg { m_lock };

	const auto enumerator = m_tree.iterate<gen::TreeIteration::InOrder>();
	auto it = enumerator.begin();
	while(it != enumerator.end()) {
		const auto task = container_of(*it, core::task::Task, runlist_node);
		if(task->state == task::State::Ready) {
			return task;
		}
		++it;
	}
	return nullptr;
}
