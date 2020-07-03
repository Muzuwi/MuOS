#pragma once
#include <Kernel/Interrupt/IRQSubscriber.hpp>

extern "C" void  _kernel_irq_dispatch(unsigned);
class IRQDispatcher {
	friend class IRQSubscriber;
	friend void _kernel_irq_dispatch(unsigned irq);
protected:
	static void dispatch_irq(uint8_t irq);
	static void register_subscriber(IRQSubscriber* subscriber, SubscriberPriority);
	static void remove_subscriber(IRQSubscriber* subscriber);
};