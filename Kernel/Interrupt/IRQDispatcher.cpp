#include <Kernel/Debug/kdebugf.hpp>
#include <Arch/i386/PortIO.hpp>
#include <Kernel/Interrupt/IRQDispatcher.hpp>

static IRQDispatcher::HandlerFunc s_interrupt_handlers[256] {};

extern "C"
void _kernel_irq_dispatch(unsigned irq, void* interrupt_trap_frame) {
	if(irq > 32 + 7)
		out(0xa0, 0x20);
	out(0x20, 0x20);

	IRQDispatcher::dispatch_irq(irq, interrupt_trap_frame);
}

void IRQDispatcher::dispatch_irq(uint8_t irq, void* interrupt_trap_frame) {
	auto handler = s_interrupt_handlers[irq];
	if(handler)
		handler(interrupt_trap_frame);
}

bool IRQDispatcher::register_handler(uint8_t irq_num, IRQDispatcher::HandlerFunc handler) {
	//  FIXME: Currently only one handler per irq supported
	if(s_interrupt_handlers[irq_num])
		return false;

	s_interrupt_handlers[irq_num] = handler;
	return true;
}

void IRQDispatcher::remove_handler(uint8_t irq_num, IRQDispatcher::HandlerFunc) {
	s_interrupt_handlers[irq_num] = nullptr;
}


