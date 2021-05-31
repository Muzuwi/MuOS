#include <Arch/i386/PIT.hpp>
#include <Arch/i386/PtraceRegs.hpp>
#include <Arch/i386/PortIO.hpp>
#include <Kernel/Debug/kdebugf.hpp>
#include <Kernel/Interrupt/IRQDispatcher.hpp>
#include <Kernel/Scheduler/Scheduler.hpp>
#include <Kernel/Process/Process.hpp>
#include <LibGeneric/List.hpp>
#include <LibGeneric/Spinlock.hpp>
#include <LibGeneric/LockGuard.hpp>

struct Alarm {
	Process* m_proc;
	uint64_t m_start;
	uint64_t m_len;
};

static PIT pit;
static gen::List<Alarm> s_alarms {};
static gen::Spinlock s_alarms_lock;

/*
	Updates the reload value on channel0 PIT
*/
void update_timer_reload(uint16_t freq){
	Ports::out(PIT::port_ch0_data(), freq & 0xFF);
	Ports::out(PIT::port_ch0_data(), (freq >> 8) & 0xFF);
}

void _pit_irq0_handler(PtraceRegs*) {
	pit.tick();
	Scheduler::tick();

	for(auto it = s_alarms.begin(); it != s_alarms.end(); ++it) {
		auto& alarm = *it;
		if(alarm.m_start + alarm.m_len > PIT::milliseconds())
			continue;

//		kdebugf("running in pid=%i for alarm of process pid=%i\n", Process::current()->pid(), alarm.m_proc->pid());
		assert(alarm.m_proc->state() == ProcessState::Sleeping);
		if(alarm.m_proc->state() == ProcessState::Sleeping) {
			//  Wake up
			alarm.m_proc->set_state(ProcessState::Ready);
			//  Force rescheduling of current task
			Process::current()->force_reschedule();
		}
		s_alarms.erase(it);
	}
}

PIT::PIT() noexcept
: m_divider(1193), m_ticks(0) { //  ~1000.15 Hz
	IRQDispatcher::register_handler(0, _pit_irq0_handler);
	Ports::out(PIT::port_command(), 0b00110100);
	update_timer_reload(m_divider);
	kdebugf("[PIT] Timer at %i Hz\n", PIT::base_frequency() / m_divider);
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
	gen::LockGuard<gen::Spinlock> guard {s_alarms_lock};
	auto* proc = Process::current();
	auto time = milliseconds();
//	kdebugf("set sleep for pid=%i\n", proc->pid());

	s_alarms.push_back({.m_proc = proc, .m_start = time, .m_len = len });

}
