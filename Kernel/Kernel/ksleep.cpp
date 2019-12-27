#include <Arch/i386/timer.hpp>
#include <Kernel/ksleep.hpp>

/*
 *	Waits the specified amount of milliseconds (approximately)
 */
void ksleep(sleep_t ms) {
	auto &timer = Timer::getTimer();
	sleep_t start = (sleep_t) timer.getTimeSinceStart();
	while((sleep_t)timer.getTimeSinceStart() < ms + start)
		;
}
