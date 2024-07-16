#include <Core/IRQ/InterruptDisabler.hpp>
#include <Core/MP/MP.hpp>
#include <LibGeneric/LockGuard.hpp>
#include <Locks/KSemaphore.hpp>
#include <Process/Thread.hpp>
#include <Scheduler/Scheduler.hpp>

KSemaphore::KSemaphore(uint64 initial_value)
    : m_lock()
    , m_value(initial_value)
    , m_queue() {}

KSemaphore::~KSemaphore() {}

void KSemaphore::wait() {
	{
		core::irq::InterruptDisabler irq_disabler {};
		gen::LockGuard<gen::Spinlock> guard { m_lock };

		//  Try acquiring, if non-zero consume one and return immediately
		const auto current = m_value.load(MemoryOrdering::SeqCst);
		if(current > 0) {
			m_value.store(current - 1, MemoryOrdering::SeqCst);
			return;
		}

		//  Go to sleep, after we're woken up
		m_queue.push_back(this_cpu()->current_thread());
	}
	//  FIXME: Lost wake-up problem
	this_cpu()->scheduler->block();
}

void KSemaphore::signal() {
	core::irq::InterruptDisabler irq_disabler {};
	gen::LockGuard<gen::Spinlock> guard { m_lock };

	if(!m_queue.empty()) {
		auto* thread = m_queue.front();
		m_queue.pop_front();
		this_cpu()->scheduler->wake_up(thread);
		return;
	}
	const auto current = m_value.load(MemoryOrdering::SeqCst);
	m_value.store(current + 1, MemoryOrdering::SeqCst);
}
