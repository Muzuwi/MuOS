#pragma once
#include <Core/Task/Task.hpp>

namespace core::task {
	///  RAII wrapper for preventing preemption of the given task
	class PreemptGuard {
	public:
		constexpr PreemptGuard(core::task::Task* task)
		    : m_task(task) {
			core::task::preempt_disable(m_task);
		}

		constexpr ~PreemptGuard() { core::task::preempt_enable(m_task); }
	private:
		core::task::Task* m_task;
	};
}
