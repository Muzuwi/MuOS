#include <Arch/x86_64/PIT.hpp>
#include <Core/MP/MP.hpp>
#include <Process/Process.hpp>
#include <Process/Thread.hpp>
#include <Scheduler/Scheduler.hpp>

Thread::Thread(SharedPtr<Process> parent, tid_t tid)
    : m_parent(parent)
    , m_tid(tid) {}

Thread* Thread::current() {
	return this_cpu()->current_thread();
}

void Thread::msleep(uint64 ms) {
	preempt_disable();

	PIT::sleep(ms);
	this_cpu()->scheduler->sleep();

	preempt_enable();
}

void Thread::sys_msleep(uint64 ms) {
	auto thread = this_cpu()->current_thread();
	thread->msleep(ms);
}
