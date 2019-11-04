#pragma once
#include <stdint.h>

class Timer {
	static Timer* instance;
	uint16_t m_frequency;
	uint64_t m_ticks;
public:
	static Timer& getTimer();
	Timer();
	void tick();
	unsigned int getTicks() const;
	double getTimeSinceStart() const;
	uint16_t getFrequency() const;

	//  getTicks
	//  getTimeInMs
	//  setFrequency
	//  ...

};