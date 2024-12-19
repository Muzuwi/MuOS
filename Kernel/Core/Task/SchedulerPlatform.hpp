#pragma once

namespace core::sched {
	/** Tick the scheduler
	 *
	 *  Called by the platform from a timer interrupt to
	 *  drive the scheduler's preemption.
	 */
	void tick();

	/** Run scheduler on return from an interrupt
	 *
	 *  Called by the platform when an interrupt was **done**
	 *  being serviced.
	 */
	void on_interrupt_exit();
}