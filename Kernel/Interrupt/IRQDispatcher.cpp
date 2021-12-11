#include <Arch/x86_64/PortIO.hpp>
#include <Arch/x86_64/PtraceRegs.hpp>
#include <Kernel/Interrupt/IRQDispatcher.hpp>
#include <Kernel/Scheduler/Scheduler.hpp>
#include <Kernel/SMP/SMP.hpp>
#include <LibGeneric/Spinlock.hpp>
#include <LibGeneric/LockGuard.hpp>

using gen::Spinlock;
using gen::LockGuard;

static IRQDispatcher::HandlerFunc s_interrupt_handlers[256-32] {
	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
};
static Spinlock s_handler_lock;

extern "C"
void _kernel_irq_dispatch(uint8_t irq, PtraceRegs* interrupt_trap_frame) {
	irq = irq - 32;
	if(irq > 7)
		Ports::out(0xa0, 0x20);
	Ports::out(0x20, 0x20);

	IRQDispatcher::dispatch_irq(irq, interrupt_trap_frame);

	SMP::ctb().scheduler().interrupt_return_common();
}

void IRQDispatcher::dispatch_irq(uint8_t irq, PtraceRegs* interrupt_trap_frame) {
	s_handler_lock.lock();
	auto handler = s_interrupt_handlers[irq];
	s_handler_lock.unlock();

	if(handler)
		handler(interrupt_trap_frame);
}

bool IRQDispatcher::register_handler(uint8_t irq_num, IRQDispatcher::HandlerFunc handler) {
	LockGuard<Spinlock> guard{s_handler_lock};

	//  FIXME: Currently only one handler per irq supported
	if(s_interrupt_handlers[irq_num])
		return false;

	s_interrupt_handlers[irq_num] = handler;
	return true;
}

void IRQDispatcher::remove_handler(uint8_t irq_num, IRQDispatcher::HandlerFunc) {
	LockGuard<Spinlock> guard{s_handler_lock};

	s_interrupt_handlers[irq_num] = nullptr;
}


