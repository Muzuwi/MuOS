#pragma once
#include <SystemTypes.hpp>

class PtraceRegs;
class Thread;

extern "C" void _kernel_irq_dispatch(uint8_t, PtraceRegs*);

#define DEFINE_MICROTASK(funcname) void funcname(PtraceRegs*)

class IRQDispatcher {
	friend void _kernel_irq_dispatch(uint8_t, PtraceRegs*);
	static void dispatch_irq(uint8_t irq, PtraceRegs*);
	static void dispatch_microtask(uint8_t irq, PtraceRegs*);
public:
	typedef void (*HandlerFunc)(PtraceRegs*);
	static bool register_microtask(uint8_t irq_num, HandlerFunc);
	static void remove_microtask(uint8_t irq_num, HandlerFunc);
};