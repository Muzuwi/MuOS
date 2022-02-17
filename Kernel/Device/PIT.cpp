#include <Arch/x86_64/PortIO.hpp>
#include <Arch/x86_64/PtraceRegs.hpp>
#include <Device/PIT.hpp>
#include <Interrupt/IRQDispatcher.hpp>
#include <LibGeneric/List.hpp>
#include <LibGeneric/LockGuard.hpp>
#include <LibGeneric/Spinlock.hpp>
#include <Process/Process.hpp>
#include <Process/Thread.hpp>
#include <Scheduler/Scheduler.hpp>
#include <SMP/SMP.hpp>

struct Alarm {
	Thread* m_thread;
	uint64_t m_start;
	uint64_t m_len;
};

static PIT pit;
static gen::List<Alarm> s_alarms {};
static gen::Spinlock s_alarms_lock;

/*
    Updates the reload value on channel0 PIT
*/
void update_timer_reload(uint16_t freq) {
	Ports::out(PIT::port_ch0_data(), freq & 0xFF);
	Ports::out(PIT::port_ch0_data(), (freq >> 8) & 0xFF);
}

void _pit_irq0_handler(PtraceRegs*) {
	pit.tick();
	SMP::ctb().scheduler().tick();

	for(auto it = s_alarms.begin(); it != s_alarms.end(); ++it) {
		auto& alarm = *it;
		if(alarm.m_start + alarm.m_len > PIT::milliseconds())
			continue;

		assert(alarm.m_thread->state() == TaskState::Sleeping);
		if(alarm.m_thread->state() == TaskState::Sleeping) {
			//  Wake up
			SMP::ctb().scheduler().wake_up(alarm.m_thread);
		}
		s_alarms.erase(it);
	}
}

PIT::PIT() noexcept
    : m_divider(1193)
    , m_ticks(0) {//  ~1000.15 Hz
	IRQDispatcher::register_microtask(0, _pit_irq0_handler);
	Ports::out(PIT::port_command(), 0b00110100);
	update_timer_reload(m_divider);
	//	kdebugf("[PIT] Timer at %i Hz\n", PIT::base_frequency() / m_divider);
}

void PIT::tick() {
	m_ticks++;
}

unsigned PIT::frequency() const {
	return PIT::base_frequency() / m_divider;
}

uint64_t PIT::millis() const {
	return 1000 * m_ticks / frequency();
}

uint64_t PIT::ticks() {
	return pit.m_ticks;
}

uint64_t PIT::milliseconds() {
	return pit.millis();
}

void PIT::sleep(uint64_t len) {
	gen::LockGuard<gen::Spinlock> guard { s_alarms_lock };
	auto* thread = SMP::ctb().current_thread();
	auto time = milliseconds();
	//	kdebugf("set sleep for pid=%i\n", proc->pid());

	s_alarms.push_back({ .m_thread = thread, .m_start = time, .m_len = len });
}
