#pragma once
#include <SystemTypes.hpp>

class PtraceRegs;

class PIT final {
	uint16_t m_divider;
	uint64_t m_ticks;

	void tick();
	friend void _pit_irq0_handler(PtraceRegs*);
	unsigned frequency() const;
	uint64_t millis() const;
public:
	PIT() noexcept;
	static uint64_t ticks();
	static uint64_t milliseconds();

	static constexpr uint16_t port_ch0_data() { return 0x40; }

	static constexpr uint16_t port_command() { return 0x43; }

	static constexpr unsigned base_frequency() { return 1193182; }

	static void sleep(uint64_t);
};
