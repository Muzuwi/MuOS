#include <Arch/i386/Timer.hpp>
#include <Arch/i386/PortIO.hpp>
#include <Kernel/Debug/kdebugf.hpp>
#include <Arch/i386/IRQDisabler.hpp>
#include <LibGeneric/List.hpp>
#include <Kernel/Process/Scheduler.hpp>
#include <include/Kernel/Debug/kassert.hpp>

#define BASE_FREQ 1193182

static gen::List<Alarm> s_alarms {};

/*
	Updates the reload value on channel0 PIT
*/
void update_timer_reload(uint16_t freq){
	out(0x40, freq & 0xFF);
	out(0x40, (freq >> 8) & 0xFF);
}

static void _timer_subscriber() {
	auto& timer = Timer::getTimer();
	timer.tick();

	const unsigned ticks = timer.getTicks();
	for(auto& alarm : s_alarms) {
		if(alarm.finished) continue;

		if(ticks > alarm.start && ticks - alarm.start >= alarm.sleeping_for) {
			alarm.finished = true;
			alarm.sleeping_for = 0;
			alarm.owner->wake_up();
		}
	}
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

unsigned Timer::sleep_for(unsigned int ms) {
	IRQDisabler disabler;

	Process::current()->set_state(ProcessState::Sleeping);
	s_alarms.push_back( {
				false, Timer::getTimer().getTicks(), ((ms * 1000) / Timer::getTimer().getFrequency()), Process::current()
			});
	auto alarm_it = --s_alarms.end();
	const auto& alarm = s_alarms.back();

	Scheduler::switch_task();

	unsigned ret = alarm.sleeping_for;
	s_alarms.erase(alarm_it);

	return ret;
}
