#pragma once
#include <Kernel/SystemTypes.hpp>
#include <Kernel/Interrupt/IRQDispatcher.hpp>

class Serial {
public:
	enum class Port {
		COM0 = 0
	};

	enum class Register {
		Data  = 0,
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
private:
	static void _serial_irq_handler(PtraceRegs*);
	static bool probe(Port port);
	static void initialize(Port port);
	static uint16 io_port(Port port);

	static void register_write(Port port, Register reg, uint8 val);
	static uint8 register_read(Port port, Register reg);
	static bool data_pending(Port port);
	static bool transmitter_empty(Port port);
};
