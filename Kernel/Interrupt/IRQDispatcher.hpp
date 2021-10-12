#pragma once

class PtraceRegs;

extern "C" void  _kernel_irq_dispatch(uint8_t, PtraceRegs*);

class IRQDispatcher {
	friend void  _kernel_irq_dispatch(uint8_t, PtraceRegs*);
	static void dispatch_irq(uint8_t irq, PtraceRegs*);
public:
	typedef void (*HandlerFunc)(PtraceRegs*);
	static bool register_handler(uint8_t irq_num, HandlerFunc);
	static void remove_handler(uint8_t irq_num, HandlerFunc);
};