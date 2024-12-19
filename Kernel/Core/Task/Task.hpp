#pragma once
#include <LibGeneric/AVL.hpp>
#include <LibGeneric/SharedPtr.hpp>
#include <Structs/KAtomic.hpp>
#include <SystemTypes.hpp>

namespace core::task {
	/**	Default kernel stack size for all tasks, can be manually overridden
	 */
	static constexpr size_t TASK_DEFAULT_STACK_SIZE = 4096;
	using TaskMainFn = void (*)();
	using TaskId = uint64;
	using NodeId = uint8;

	/** Priority given to individual tasks
	 *
	 *  128 levels are reserved for kernel-only tasks, to allow them
	 *  to be considered before any other userland tasks. Other priority
	 *  values can be used both by kernel and user tasks.
	 */
	enum Priority : uint8 {
		_TaskPriorityKernelBase = 0,
		/* 0..127: kernel-only prios */
		_TaskPriorityUserBase = 128,
		/* 128..254: common prios */
		_TaskPriorityIdle = 255
		/* 255: idle-task */
	};

	enum class State {
		/* task hasn't been run yet */
		New,
		/* task has been run at least once and can be freely scheduled */
		Ready,
		/* actively running */
		Running,
		/* waiting to be unblocked by a different task */
		Blocked,
		/* task will be cleaned up soon */
		Leaving,
	};

	/** Convert a priority to a count of quants that a task can use
	 */
	constexpr uint64 priority_to_quants(Priority priority) {
		if(priority < _TaskPriorityUserBase) {//  Allow kernel tasks more CPU time
			return (_TaskPriorityUserBase - priority) * 20;
		} else {
			return (priority - _TaskPriorityUserBase) * 5;
		}
	}

	/**	Returns whether priority A is to be considered more important
	 *	than priority B. In general, priority values should be used
	 * 	only via dedicated comparator functions instead of comparing
	 * 	their values manually.
	 */
	constexpr bool is_higher_priority(Priority A, Priority B) {
		return A < B;
	}

	/** Task structure
	 */
	struct Task {
		/* architecture-specific stuff */
#ifdef ARCH_IS_x86_64
		void* interrupted_task_frame {};
		void* kernel_stack_bottom {};
		void* kernel_gs_base {};
		void* pml4 {};
#endif
		core::task::TaskId id;
		/* signals if the task should be rescheduled on next exit from interrupt */
		bool need_resched;
		State state;
		Priority priority;
		KAtomic<uint64> preempt_count;
		uint64 quants_left;
		NodeId node_id;
		/* scheduler runlist intrusive node */
		gen::AvlNode runlist_node {};
		/* global tasklist intrusive node */
		gen::AvlNode tasklist_node {};
	};

	constexpr void preempt_disable(core::task::Task* task) {
		if(!task) {
			return;
		}
		(void)task->preempt_count.fetch_add(1, MemoryOrdering::SeqCst);
	}

	constexpr void preempt_enable(core::task::Task* task) {
		if(!task) {
			return;
		}
		(void)task->preempt_count.fetch_sub(1, MemoryOrdering::SeqCst);
	}

	struct TaskCreationParameters {
		const core::task::Priority priority = static_cast<Priority>(127);
		const size_t kernel_stack_size = TASK_DEFAULT_STACK_SIZE;
	};

	/**	Create a kernel task.
	 *
	 *	This creates a ready-to-run kernel task that is not yet associated with
	 * 	any particular node.
	 */
	gen::SharedPtr<core::task::Task> make(TaskMainFn, TaskCreationParameters = {});

	/**	Find a task by the given task ID.
	 *
	 *	Find the task control structure for the task with the given ID.
	 */
	//  gen::SharedPtr<core::task::Task> find_by_tid(size_t tid);
}
