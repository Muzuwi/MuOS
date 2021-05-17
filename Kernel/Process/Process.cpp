#include <Arch/i386/CPU.hpp>
#include <Arch/i386/PIT.hpp>
#include <Kernel/Debug/kassert.hpp>
#include <Kernel/Process/Process.hpp>
#include <Kernel/Memory/VMapping.hpp>
#include <Kernel/Scheduler/Scheduler.hpp>
#include <LibGeneric/LockGuard.hpp>
#include <LibGeneric/Algorithm.hpp>

using gen::Spinlock;
using gen::List;
using gen::LockGuard;

Process* Process::s_current {nullptr};
List<Process*> Process::s_process_list {};
gen::Spinlock Process::s_process_list_lock {};

Process::Process(pid_t pid, ProcessFlags flags)
: m_interrupted_task_frame(nullptr), m_kernel_stack_bottom(nullptr), m_userland_stack(nullptr), m_process_lock(),
  m_pid(pid), m_flags(flags), m_state(ProcessState::New),  m_address_space(), m_priority(0), m_quants_left(0),
  m_preempt_count(0), m_kernel_gs_base(0)
{
}

Process::~Process() {
	assert(m_state == ProcessState::Leaving);
	LockGuard<Spinlock> plist_lock {s_process_list_lock};
	auto it = gen::find(s_process_list, this);
	s_process_list.erase(it);
}

Process* Process::current() {
	return s_current;
}

pid_t Process::pid() const {
	return m_pid;
}

ProcessState Process::state() const {
	return m_state;
}

ProcessFlags Process::flags() const {
	return m_flags;
}

PhysPtr<PML4> Process::pml4() const {
	return m_pml4;
}

uint8_t Process::priority() const {
	return m_priority;
}

InactiveTaskFrame* Process::frame() const {
	return m_interrupted_task_frame;
}

void Process::set_state(ProcessState state) {
	m_state = state;
}

void Process::force_reschedule() {
	m_flags.need_resched = 1;
}


void Process::msleep(uint64_t ms) {
	CPU::irq_disable();
	gen::LockGuard<gen::Spinlock> lock {m_process_lock};
	m_state = ProcessState::Sleeping;
	PIT::sleep(ms);

	Scheduler::schedule();
	CPU::irq_enable();
}

void Process::start() {
	gen::LockGuard<Spinlock> lock {s_process_list_lock};
	s_process_list.push_back(this);
	Scheduler::notify_process_start(this);
}

ProcMem* Process::memory() {
	return &m_address_space;
}

void Process::preempt_enable() {
	__atomic_add_fetch(&m_preempt_count, -1, __ATOMIC_SEQ_CST);
}

void Process::preempt_disable() {
	__atomic_add_fetch(&m_preempt_count, 1, __ATOMIC_SEQ_CST);
}

uint64_t Process::preempt_count() const {
	return __atomic_load_n(&m_preempt_count, __ATOMIC_SEQ_CST);
}

void Process::self_lock() {
	auto* process = Process::current();
	process->m_process_lock.lock();
}

void Process::self_unlock() {
	auto* process = Process::current();
	process->m_process_lock.unlock();
}
