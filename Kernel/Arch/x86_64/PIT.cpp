#include <Arch/x86_64/PIT.hpp>
#include <Arch/x86_64/PortIO.hpp>
#include <Arch/x86_64/PtraceRegs.hpp>
#include <Core/Assert/Assert.hpp>
#include <Core/IRQ/IRQ.hpp>
#include <Core/Log/Logger.hpp>
#include <Core/MP/MP.hpp>
#include <Core/Task/Scheduler.hpp>
#include <Core/Task/SchedulerPlatform.hpp>
#include <LibGeneric/List.hpp>
#include <LibGeneric/LockGuard.hpp>
#include <LibGeneric/Spinlock.hpp>

CREATE_LOGGER("x86_64::pit", core::log::LogLevel::Debug);

struct Alarm {
	core::sched::BlockHandle m_handle;
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
	core::sched::tick();

	for(auto it = s_alarms.begin(); it != s_alarms.end(); ++it) {
		auto& alarm = *it;
		if(alarm.m_start + alarm.m_len > PIT::milliseconds()) {
			continue;
		}
		//  Wake up the sleeping task
		core::sched::unblock(alarm.m_handle);
		s_alarms.erase(it);
	}
}

PIT::PIT() noexcept
    : m_divider(1193)
    , m_ticks(0) {}

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
	Alarm* ref;
	{
		gen::LockGuard<gen::Spinlock> guard { s_alarms_lock };
		auto time = milliseconds();

		s_alarms.push_back({ nullptr, time, len });
		ref = &s_alarms.back();
	}
	core::sched::block(ref->m_handle);
}

void x86_64::pit_init() {
	//  ~1000.15 Hz
	const auto maybe_handle = core::irq::request_irq(32 + 0,
	                                                 [](void*) -> core::irq::HandlingState {
		                                                 _pit_irq0_handler(nullptr);
		                                                 return core::irq::HandlingState::Handled;
	                                                 },
	                                                 {});
	if(maybe_handle.has_error()) {
		::log.error("Failed to request IRQ for PIT ({})", maybe_handle.error());
		return;
	}
	Ports::out(PIT::port_command(), 0b00110100);
	update_timer_reload(pit.m_divider);
}