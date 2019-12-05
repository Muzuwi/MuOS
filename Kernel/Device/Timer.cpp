#include <arch/i386/timer.hpp>
#include <arch/i386/portio.hpp>
#include <kernel/kdebugf.hpp>

#define assert(a)
#define BASE_FREQ 1193182

Timer* Timer::instance = nullptr; 
static Timer system_timer;

/*
	Updates the reload value on channel0 PIT
*/
void update_timer_reload(uint16_t freq){
	out(0x40, freq & 0xFF);
	out(0x40, (freq >> 8) & 0xFF);
}


Timer::Timer(){
	if(Timer::instance){
		kerrorf("Only one timer instance allowed!\n");
		return;
	}
	Timer::instance = this;
	m_frequency = 1000;
	out(0x43, 0b00110110);
	update_timer_reload(BASE_FREQ/m_frequency);
	kdebugf("[timer] Timer initialized at %i Hz\n", m_frequency);
	
}

/*
	Returns a timer instance
*/
Timer& Timer::getTimer(){
	//  TODO: Asserts
	assert(Timer::instance);

	return *Timer::instance;
}

/*
	Called by the irq handler to notify a tick
	has occured
*/
void Timer::tick(){
	m_ticks++;
}

/*
	Returns the amount of ticks since timer
	start
*/
unsigned int Timer::getTicks() const{
	return m_ticks;
}
	
/*
	Gets the time in milliseconds since timer 
	was started
*/
double Timer::getTimeSinceStart() const{
	return m_ticks / ((double)m_frequency / 1000);
}

/*
	Gets the current timer frequency
*/
uint16_t Timer::getFrequency() const {
	return m_frequency;
}