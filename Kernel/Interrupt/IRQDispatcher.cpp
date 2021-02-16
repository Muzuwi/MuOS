#include <Arch/i386/PortIO.hpp>
#include <Kernel/Debug/kdebugf.hpp>
#include <Kernel/Interrupt/IRQDispatcher.hpp>
#include <LibGeneric/Mutex.hpp>
#include <LibGeneric/LockGuard.hpp>

using gen::Mutex;
using gen::LockGuard;

static IRQDispatcher::HandlerFunc s_interrupt_handlers[256] {
	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
};
static Mutex s_handler_mutex;

extern "C"
void _kernel_irq_dispatch(uint8_t irq, void* interrupt_trap_frame) {
	if(irq > 32 + 7)
		out(0xa0, 0x20);
	out(0x20, 0x20);

	IRQDispatcher::dispatch_irq(irq, interrupt_trap_frame);
}

void IRQDispatcher::dispatch_irq(uint8_t irq, void* interrupt_trap_frame) {
	s_handler_mutex.lock();
	auto handler = s_interrupt_handlers[irq];
	s_handler_mutex.unlock();

	if(handler)
		handler(interrupt_trap_frame);
}

bool IRQDispatcher::register_handler(uint8_t irq_num, IRQDispatcher::HandlerFunc handler) {
	LockGuard<Mutex> guard{s_handler_mutex};

	//  FIXME: Currently only one handler per irq supported
	if(s_interrupt_handlers[irq_num])
		return false;

	s_interrupt_handlers[irq_num] = handler;
	return true;
}

void IRQDispatcher::remove_handler(uint8_t irq_num, IRQDispatcher::HandlerFunc) {
	LockGuard<Mutex> guard{s_handler_mutex};

	s_interrupt_handlers[irq_num] = nullptr;
}


