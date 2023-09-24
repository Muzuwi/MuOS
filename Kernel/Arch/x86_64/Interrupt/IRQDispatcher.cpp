#include "IRQDispatcher.hpp"
#include <Arch/x86_64/CPU.hpp>
#include <Arch/x86_64/IRQDisabler.hpp>
#include <Arch/x86_64/PortIO.hpp>
#include <Arch/x86_64/PtraceRegs.hpp>
#include <LibGeneric/Algorithm.hpp>
#include <LibGeneric/LockGuard.hpp>
#include <LibGeneric/Spinlock.hpp>
#include <Scheduler/Scheduler.hpp>
#include <SMP/SMP.hpp>

using gen::LockGuard;
using gen::Spinlock;

static IRQDispatcher::HandlerFunc s_microtasks[256 - 32] {};
static Spinlock s_microtask_lock {};

static gen::List<Thread*> s_threadirqs[256 - 32] {};
static Spinlock s_threadirqs_lock {};

extern "C" void _kernel_irq_dispatch(uint8_t irq, PtraceRegs* interrupt_trap_frame) {
	irq = irq - 32;
	if(irq > 7)
		Ports::out(0xa0, 0x20);
	Ports::out(0x20, 0x20);

	IRQDispatcher::dispatch_irq(irq, interrupt_trap_frame);

	SMP::ctb().scheduler().interrupt_return_common();
}

void IRQDispatcher::dispatch_irq(uint8_t irq, PtraceRegs* interrupt_trap_frame) {
	dispatch_microtask(irq, interrupt_trap_frame);
}

void IRQDispatcher::dispatch_microtask(uint8_t irq, PtraceRegs* interrupt_trap_frame) {
	s_microtask_lock.lock();
	auto handler = s_microtasks[irq];
	if(handler) {
		handler(interrupt_trap_frame);
	}
	s_microtask_lock.unlock();
}

/*
 *  Registers a microtask to run for the specified IRQ.
 *  A microtask is a very small handler running immediately in an IRQ context,
 *  and care must be taken to not introduce deadlocks. Only one microtask may
 *  be registered for a single IRQ.
 */
bool IRQDispatcher::register_microtask(uint8_t irq_num, IRQDispatcher::HandlerFunc handler) {
	IRQDisabler disabler {};
	s_microtask_lock.lock();

	bool retval = false;
	if(s_microtasks[irq_num] == nullptr) {
		s_microtasks[irq_num] = handler;
		retval = true;
	}

	s_microtask_lock.unlock();
	return retval;
}

/*
 *  Removes a microtask from the specified IRQ.
 */
void IRQDispatcher::remove_microtask(uint8_t irq_num, IRQDispatcher::HandlerFunc handler) {
	IRQDisabler disabler {};
	s_microtask_lock.lock();

	if(s_microtasks[irq_num] == handler) {
		s_microtasks[irq_num] = nullptr;
	}

	s_microtask_lock.unlock();
}
