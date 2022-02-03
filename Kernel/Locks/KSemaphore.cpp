#include <Arch/x86_64/IRQDisabler.hpp>
#include <Locks/KSemaphore.hpp>
#include <LibGeneric/LockGuard.hpp>
#include <SMP/SMP.hpp>
#include <Process/Thread.hpp>

KSemaphore::KSemaphore(uint64 initial_value)
		: m_lock(), m_value(initial_value), m_queue() {

}

KSemaphore::~KSemaphore() {

}

void KSemaphore::wait() {
	{
		IRQDisabler irq_disabler {};
		gen::LockGuard<gen::Spinlock> guard { m_lock };

		//  Try acquiring, if non-zero consume one and return immediately
		const auto current = m_value.load(MemoryOrdering::SeqCst);
		if(current > 0) {
			m_value.store(current - 1, MemoryOrdering::SeqCst);
			return;
		}

		//  Go to sleep, after we're woken up
		m_queue.push_back(SMP::ctb().current_thread());
	}
	//  FIXME: Lost wake-up problem
	SMP::ctb().current_thread()->set_state(TaskState::Blocking);
	SMP::ctb().scheduler().schedule();
}

void KSemaphore::signal() {
	IRQDisabler irq_disabler {};
	gen::LockGuard<gen::Spinlock> guard { m_lock };

	if(!m_queue.empty()) {
		auto* thread = m_queue.front();
		m_queue.pop_front();
		SMP::ctb().scheduler().wake_up(thread);
		return;
	}
	const auto current = m_value.load(MemoryOrdering::SeqCst);
	m_value.store(current + 1, MemoryOrdering::SeqCst);
}
