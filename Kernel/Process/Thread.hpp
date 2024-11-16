#pragma once
#include <Arch/VM.hpp>
#include <Arch/x86_64/PtraceRegs.hpp>
#include <LibGeneric/SharedPtr.hpp>
#include <SystemTypes.hpp>

enum class TaskState {
	New,
	Ready,
	Running,
	Blocking,
	Leaving,
	Sleeping
};

struct TaskFlags {
	uint8 need_resched    : 1;
	uint8 uninterruptible : 1;
};

struct TaskSchedCtx {
	uint8 priority;
	uint64 preempt_count;
	uint64 quants_left;
};

using gen::SharedPtr;

class InactiveTaskFrame;
class Process;

extern "C" void _task_enter_bootstrap();

class Thread {
	friend class Process;
	friend class Scheduler;

	//  Layout of the following fields is important, as we're directly accessing
	//  these from asm code
	InactiveTaskFrame* m_interrupted_task_frame {};//  Offset 0x0
	void* m_kernel_stack_bottom {};                //  Offset 0x8
	uint64 m_kernel_gs_base {};                    //  Offset 0x10
	arch::PagingHandle m_paging_handle {};         //  Offset 0x18
	//  ==================

	SharedPtr<Process> m_parent {};
	tid_t m_tid {};

	TaskState m_state { TaskState::New };
	TaskFlags m_flags {};
	TaskSchedCtx m_sched {};

	Thread(SharedPtr<Process>, tid_t);

	[[maybe_unused]] static void finalize_switch(Thread* prev, Thread* next);
public:
	static SharedPtr<Thread> create_in_process(SharedPtr<Process>, void (*kernel_exec)());
	static Thread* current();

	tid_t tid() const { return m_tid; }
	SharedPtr<Process> const& parent() const { return m_parent; }
	TaskState state() const { return m_state; }
	TaskFlags const& flags() const { return m_flags; }
	TaskSchedCtx& sched_ctx() { return m_sched; }
	uint8 priority() const { return m_sched.priority; }
	InactiveTaskFrame* irq_task_frame() const { return m_interrupted_task_frame; }
	arch::PagingHandle paging_handle() const { return m_paging_handle; }

	void set_state(TaskState state) { m_state = state; }

	void preempt_disable() { __atomic_add_fetch(&m_sched.preempt_count, 1, __ATOMIC_SEQ_CST); }
	void preempt_enable() { __atomic_add_fetch(&m_sched.preempt_count, -1, __ATOMIC_SEQ_CST); }
	uint64 preempt_count() { return __atomic_load_n(&m_sched.preempt_count, __ATOMIC_SEQ_CST); }
	void reschedule() { m_flags.need_resched = true; }
	bool needs_reschedule() { return m_flags.need_resched; }
	void clear_reschedule() { m_flags.need_resched = false; }

	void msleep(uint64 ms);

	static void sys_msleep(uint64);
};