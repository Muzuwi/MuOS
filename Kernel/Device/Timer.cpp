#include <Arch/i386/Timer.hpp>
#include <Arch/i386/PortIO.hpp>
#include <Kernel/Debug/kdebugf.hpp>
#include <Arch/i386/IRQDisabler.hpp>

#define BASE_FREQ 1193182

/*
	Updates the reload value on channel0 PIT
*/
void update_timer_reload(uint16_t freq){
	out(0x40, freq & 0xFF);
	out(0x40, (freq >> 8) & 0xFF);
}

static void _timer_subscriber() {
	Timer::getTimer().tick();
}

Timer::Timer()
: IRQSubscriber(0, _timer_subscriber) {
	IRQDisabler disabler;
	m_frequency = 1000;
	out(0x43, 0b00110100);
	update_timer_reload(BASE_FREQ/m_frequency);
	kdebugf("[timer] Timer initialized at %i Hz\n", m_frequency);
}

/*
	Returns a timer instance
*/
Timer& Timer::getTimer(){
	static Timer system_timer;
	return system_timer;
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