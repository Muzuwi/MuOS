#pragma once
#include <LibGeneric/Spinlock.hpp>
#include <SystemTypes.hpp>

struct PtraceRegs;

class PIT final {
public:
	enum class Mode {
		Mode0 = 0,
		Mode1 = 1,
		Mode2 = 2,
		Mode3 = 3,
		Mode4 = 4,
		Mode5 = 5,
		Mode2x6Alias = 6,
		Mode3x7Alias = 7,
	};
private:
	static PIT s_instance;
	static constexpr unsigned base_clock = 1193182;
	static constexpr uint16 port_ch0_data = 0x40;
	static constexpr uint16 port_ch0_command = 0x43;

	static void pit_irq_handler(PtraceRegs*);

	gen::Spinlock m_lock;
	uint16 m_divider;
	volatile uint64 m_ticks;

	void set_divider(uint16 divider);
	void reconfigure(PIT::Mode, uint16 frequency);

	constexpr uint64 frequency() const { return PIT::base_clock / m_divider; }

	PIT() noexcept;
public:
	static void init_with_frequency(uint16 frequency);
	static uint64 milliseconds();
	static void sleep(uint64);
};
