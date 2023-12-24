#include <Arch/x86_64/PortIO.hpp>
#include <Arch/x86_64/Serial.hpp>
#include <Core/Log/Logger.hpp>
#include <string.h>
#include <Structs/KOptional.hpp>

CREATE_LOGGER("x86_64::i8250", core::log::LogLevel::Debug);

//  FIXME: Only support COM0 for now

static KOptional<Serial::Port> s_kernel_debugger_port {};
static KSemaphore s_debugger_semaphore {};
static StaticRing<uint8, 4096> s_buffer;

void Serial::set_debugger_port(Serial::Port port) {
	if(s_kernel_debugger_port.has_value()) {
		return;
	}

	s_kernel_debugger_port = KOptional<Serial::Port> { port };
	//  Enable IRQs for the kernel debugger - data inbound
	register_write(port, Register::IrqEn, 0x1);
	//  Register microtask
	IRQDispatcher::register_microtask(irq(port), _serial_irq_handler);
}

void Serial::_serial_irq_handler(PtraceRegs*) {
	const auto port = s_kernel_debugger_port.unwrap();
	bool data_received = false;
	while(data_pending(port)) {
		auto data = register_read(port, Register::Data);
		s_buffer.try_push(data);
		data_received = true;
	}
	if(data_received) {
		s_debugger_semaphore.signal();
	}
}

bool Serial::try_initialize(Serial::Port port) {
	const uint8 magic = 0x7f;

	//  Try using the scratch pad
	register_write(port, Register::Scratch, magic);
	const auto value = register_read(port, Register::Scratch);
	if(value != magic) {
		return false;
	}

	//  Set speed - 115200
	register_write(port, Register::LineControl, 0x80);//  DLAB on
	register_write(port, Register::Data, 0x01);       //  Divisor 1
	register_write(port, Register::IrqEn, 0x00);
	register_write(port, Register::LineControl, 0x03);//  8N1, DLAB off

	register_write(port, Register::IrqEn, 0x0);
	register_write(port, Register::IrqId, 0x01);       //  Enable FIFO
	register_write(port, Register::IrqId, 0x06);       //  Explicitly clear both FIFOs
	register_write(port, Register::ModemControl, 0x0b);//  RTS/DTS set, irq enabled

	//  FIXME: This is faulty.
	//  On real hardware, the loopback test for whatever reason doesn't work.
	//  When cold-booting, it doesn't seem to pass, and after doing a reboot (! not shutdown and power on !),
	//  it seems to come back fine (interestingly, the loopback byte seems to be sent outside of the port, is it
	//  supposed to be like that?)
	//  Conveniently ignore the loopback mode for now.

	//  Test if the serial port actually exists
	//	register_write(port, Register::ModemControl, 0x10);//  Loopback mode
	//  Clear any potential pending data
	//	for(unsigned i = 0; i < 30; ++i)
	//		(void)register_read(port, Register::Data);
	//	register_write(port, Register::Data, magic);
	//	const uint8 read = register_read(port, Register::Data);
	//	if(read != magic) {
	//		return false;
	//	}
	//  Return to normal operation mode
	//	register_write(port, Register::ModemControl, 0x0B);

	return true;
}

void Serial::init() {
	auto do_init = [](Serial::Port port) {
		if(try_initialize(port)) {
			set_debugger_port(port);
			log.info("Initialized COM{}, speed 115200, 8N1, IRQ 4", (unsigned)port);
		}
	};

	do_init(Port::COM0);
	do_init(Port::COM1);
	do_init(Port::COM2);
	do_init(Port::COM3);
}

uint16 Serial::io_port(Serial::Port port) {
	static constexpr const uint16 io_address_for_port[4] = { 0x3F8, 0x2F8, 0x3E8, 0x2E8 };
	return io_address_for_port[static_cast<size_t>(port) % 4];
}

void Serial::register_write(Serial::Port port, Serial::Register reg, uint8 val) {
	Ports::out(io_port(port) + static_cast<uint16>(reg), val);
}

uint8 Serial::register_read(Serial::Port port, Serial::Register reg) {
	return Ports::in(io_port(port) + static_cast<uint16>(reg));
}

bool Serial::data_pending(Serial::Port port) {
	return register_read(port, Register::LineStatus) & 1u;
}

bool Serial::transmitter_empty(Serial::Port port) {
	return register_read(port, Register::LineStatus) & (1u << 5u);
}

void Serial::write_str(Serial::Port port, char const* str) {
	for(unsigned i = 0; i < strlen(str); ++i) {
		while(!transmitter_empty(port))
			;
		register_write(port, Register::Data, str[i]);
	}
}

void Serial::write_debugger_str(char const* str) {
	if(!s_kernel_debugger_port.has_value()) {
		return;
	}

	write_str(s_kernel_debugger_port.unwrap(), str);
}

StaticRing<uint8, 4096>& Serial::buffer() {
	return s_buffer;
}

uint8 Serial::irq(Serial::Port port) {
	static constexpr const uint16 io_address_for_port[4] = { 4, 3, 4, 3 };
	return io_address_for_port[static_cast<size_t>(port) % 4];
}

KSemaphore& Serial::debugger_semaphore() {
	return s_debugger_semaphore;
}
