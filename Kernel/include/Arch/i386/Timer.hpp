#pragma once
#include <stdint.h>
#include <Kernel/Interrupt/IRQSubscriber.hpp>

class Process;

class Timer final : protected IRQSubscriber {
	uint16_t m_frequency;
	uint64_t m_ticks;
public:
	static Timer& getTimer();
	Timer();
	void tick();
	unsigned int getTicks() const;
	double getTimeSinceStart() const;
	uint16_t getFrequency() const;

	static unsigned sleep_for(unsigned ms);

	//  getTicks
	//  getTimeInMs
	//  setFrequency
	//  ...

};

struct Alarm {
	bool finished;
	unsigned start;
	unsigned sleeping_for;
	Process* owner;
};