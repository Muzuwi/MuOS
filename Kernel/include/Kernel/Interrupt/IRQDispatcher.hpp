#pragma once
#include <Arch/i386/TrapFrame.hpp>
#include <Kernel/Interrupt/IRQSubscriber.hpp>

extern "C" void  _kernel_irq_dispatch(unsigned, TrapFrame);
class IRQDispatcher {
	friend class IRQSubscriber;
	friend void _kernel_irq_dispatch(unsigned irq, TrapFrame trapFrame);
protected:
	static void dispatch_irq(uint8_t irq, TrapFrame& frame);
	static void register_subscriber(IRQSubscriber* subscriber);
	static void remove_subscriber(IRQSubscriber* subscriber);
};