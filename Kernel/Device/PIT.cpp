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

PIT PIT::s_instance {};
static gen::List<Alarm> s_alarms {};
static gen::Spinlock s_alarms_lock;

void PIT::pit_irq_handler(PtraceRegs*) {
	s_instance.m_ticks++;

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

void PIT::sleep(uint64_t len) {
	gen::LockGuard<gen::Spinlock> guard { s_alarms_lock };
	auto* thread = SMP::ctb().current_thread();
	auto time = milliseconds();
	//	kdebugf("set sleep for pid=%i\n", proc->pid());

	s_alarms.push_back({ .m_thread = thread, .m_start = time, .m_len = len });
}

PIT::PIT() noexcept
    : m_divider(1)
    , m_ticks(0) {
	IRQDispatcher::register_microtask(0, PIT::pit_irq_handler);
}

void PIT::set_divider(uint16 divider) {
	Ports::out(PIT::port_ch0_data, divider & 0xFF);
	Ports::out(PIT::port_ch0_data, (divider >> 8) & 0xFF);
	m_divider = divider;
}

void PIT::reconfigure(PIT::Mode mode, uint16 frequency) {
	const auto byte = 0b00110000 | (static_cast<uint8>(mode) << 1u);
	Ports::out(PIT::port_ch0_command, byte);

	const auto divider = base_clock / frequency;
	set_divider(divider);

	m_ticks = 0;
}

void PIT::init_with_frequency(uint16 frequency) {
	gen::LockGuard<gen::Spinlock> guard { s_instance.m_lock };
	s_instance.reconfigure(Mode::Mode2, frequency);
}

uint64 PIT::milliseconds() {
	return 1000 * s_instance.m_ticks / s_instance.frequency();
}
