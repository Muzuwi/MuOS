#include <Arch/i386/PIT.hpp>
#include <Arch/i386/PtraceRegs.hpp>
#include <Arch/i386/PortIO.hpp>
#include <Kernel/Debug/kdebugf.hpp>
#include <Kernel/Interrupt/IRQDispatcher.hpp>

static PIT pit;

/*
	Updates the reload value on channel0 PIT
*/
void update_timer_reload(uint16_t freq){
	out(PIT::port_ch0_data(), freq & 0xFF);
	out(PIT::port_ch0_data(), (freq >> 8) & 0xFF);
}

void _pit_irq0_handler(PtraceRegs*) {
	pit.tick();
}

PIT::PIT() noexcept
: m_divider(1193), m_ticks(0) { //  ~1000.15 Hz
	IRQDispatcher::register_handler(0, _pit_irq0_handler);
	out(PIT::port_command(), 0b00110100);
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
