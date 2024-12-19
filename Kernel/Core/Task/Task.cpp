#include <Arch/MP.hpp>
#include <Core/Error/Error.hpp>
#include <Core/Log/Logger.hpp>
#include <Core/Mem/VM.hpp>
#include <Core/Task/Scheduler.hpp>
#include <Core/Task/Task.hpp>
#include <LibGeneric/AVL.hpp>
#include <LibGeneric/List.hpp>
#include <LibGeneric/LockGuard.hpp>
#include <LibGeneric/SharedPtr.hpp>
#include <LibGeneric/Spinlock.hpp>
#include <SystemTypes.hpp>

class TidAllocator {
public:
	core::task::TaskId next() {
		gen::LockGuard lg { m_lock };
		return ++m_current_tid;
	}

	void free(core::task::TaskId tid) {
		//  TODO: Implement freeing of tid's
		(void)tid;
	}
private:
	///  Protects the current task ID value
	gen::Spinlock m_lock {};
	///  Task ID that was allocated to the last task
	core::task::TaskId m_current_tid {};
} s_tid;

class GlobalTaskList {
public:
	core::Error track(gen::SharedPtr<core::task::Task> task) {
		gen::LockGuard lg { m_lock };
		m_tasklist.push_back(task);
		return core::Error::Ok;
	}
private:
	///  Global tasklist lock, all modifications/reads to the tasklist must first take this lock.
	gen::Spinlock m_lock;
	///  The tasklist itself
	///  FIXME: Use an intrusive list instead
	gen::List<gen::SharedPtr<core::task::Task>> m_tasklist;
} s_tasklist;

gen::SharedPtr<core::task::Task> core::task::make(TaskMainFn kernel_exec, TaskCreationParameters params) {
	auto task = gen::make_shared<core::task::Task>();
	if(!task) {
		return {};
	}
	task->id = s_tid.next();
	task->state = core::task::State::Ready;
	task->priority = params.priority;

	auto* task_stack = core::mem::vmalloc(params.kernel_stack_size);
	if(!task_stack) {
		return {};
	}
	auto err = arch::mp::prepare_task(task.get(), kernel_exec, task_stack, params.kernel_stack_size);
	if(err != Error::Ok) {
		core::mem::vfree(task_stack);
		return {};
	}
	err = s_tasklist.track(task);
	if(err != Error::Ok) {
		core::mem::vfree(task_stack);
		return {};
	}
	return { task };
}
