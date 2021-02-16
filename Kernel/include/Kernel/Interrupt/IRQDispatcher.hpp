#pragma once

extern "C" void  _kernel_irq_dispatch(uint8_t, void*);

class IRQDispatcher {
	friend void  _kernel_irq_dispatch(uint8_t, void*);
	static void dispatch_irq(uint8_t irq, void*);
public:
	typedef void (*HandlerFunc)(void*);
	static bool register_handler(uint8_t irq_num, HandlerFunc);
	static void remove_handler(uint8_t irq_num, HandlerFunc);
};