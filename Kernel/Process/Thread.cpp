#include <Device/PIT.hpp>
#include <Process/Thread.hpp>
#include <Process/Process.hpp>
#include <SMP/SMP.hpp>

Thread::Thread(SharedPtr<Process> parent, tid_t tid) {
	m_tid = tid;
	m_parent = parent;
}

Thread* Thread::current() {
	return SMP::ctb().current_thread();
}

void Thread::msleep(uint64 ms) {
	preempt_disable();

	m_state = TaskState::Sleeping;
	PIT::sleep(ms);
	SMP::ctb().scheduler().schedule();

	preempt_enable();
}

void Thread::sys_msleep(uint64 ms) {
	auto thread = SMP::ctb().current_thread();
	thread->msleep(ms);
}
