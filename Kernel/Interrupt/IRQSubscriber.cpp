#include <Kernel/Interrupt/IRQSubscriber.hpp>
#include <Kernel/Interrupt/IRQDispatcher.hpp>

IRQSubscriber::IRQSubscriber(uint8_t irq, HandlerFunc func, SubscriberPriority priority)
: irqNumber(irq), handlerFunction(func) {
	IRQDispatcher::register_subscriber(this, priority);
}

IRQSubscriber::HandlerFunc IRQSubscriber::handler() {
	return this->handlerFunction;
}

IRQSubscriber::~IRQSubscriber() {
	IRQDispatcher::remove_subscriber(this);
}