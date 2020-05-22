#pragma once
#include <Arch/i386/TrapFrame.hpp>

class IRQSubscriber {
public:
	typedef void (*HandlerFunc)(const TrapFrame&);
private:
	uint8_t irqNumber;
	HandlerFunc handlerFunction;
public:
	IRQSubscriber(uint8_t irq, HandlerFunc func);
	~IRQSubscriber();
	HandlerFunc handler();
	uint8_t irq() const { return irqNumber; }
};