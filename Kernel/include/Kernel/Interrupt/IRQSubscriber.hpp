#pragma once
#include <stdint.h>

enum class SubscriberPriority {
	Normal,
	MayTaskSwitch   //  Informs that the subscriber should run last, because it can lead to a task switch
};

class IRQSubscriber {
public:
	typedef void (*HandlerFunc)();
private:
	uint8_t irqNumber;
	HandlerFunc handlerFunction;
public:
	IRQSubscriber(uint8_t irq, HandlerFunc func, SubscriberPriority=SubscriberPriority::Normal);
	~IRQSubscriber();
	[[nodiscard]] HandlerFunc handler();
	uint8_t irq() const { return irqNumber; }
};