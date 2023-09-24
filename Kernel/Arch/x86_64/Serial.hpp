#pragma once
#include <Arch/x86_64/Interrupt/IRQDispatcher.hpp>
#include <Locks/KSemaphore.hpp>
#include <Structs/StaticRing.hpp>
#include <SystemTypes.hpp>

class Serial {
public:
	enum class Port {
		COM0 = 0,
		COM1 = 1,
		COM2 = 2,
		COM3 = 3
	};

	enum class Register {
		Data = 0,
		IrqEn = 1,
		IrqId = 2,
		LineControl = 3,
		ModemControl = 4,
		LineStatus = 5,
		ModemStatus = 6,
		Scratch = 7
	};

	static void init();
	static void write_str(Port, const char*);
	static void write_debugger_str(const char*);
	static void set_debugger_port(Port);
	static KSemaphore& debugger_semaphore();
	static StaticRing<uint8, 4096>& buffer();
private:
	static void _serial_irq_handler(PtraceRegs*);
	static bool try_initialize(Port port);
	static uint16 io_port(Port port);
	static uint8 irq(Port port);

	static void register_write(Port port, Register reg, uint8 val);
	static uint8 register_read(Port port, Register reg);
	static bool data_pending(Port port);
	static bool transmitter_empty(Port port);
};
