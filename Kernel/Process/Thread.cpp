#include <Arch/x86_64/PIT.hpp>
#include <Process/Process.hpp>
#include <Process/Thread.hpp>
#include <SMP/SMP.hpp>

Thread::Thread(SharedPtr<Process> parent, tid_t tid)
    : m_parent(parent)
    , m_tid(tid) {}

Thread* Thread::current() {
	return SMP::ctb().current_thread();
}

void Thread::msleep(uint64 ms) {
	preempt_disable();

	PIT::sleep(ms);
	SMP::ctb().scheduler().sleep();

	preempt_enable();
}

void Thread::sys_msleep(uint64 ms) {
	auto thread = SMP::ctb().current_thread();
	thread->msleep(ms);
}
