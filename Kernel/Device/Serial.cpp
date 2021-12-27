#include <Arch/x86_64/PortIO.hpp>
#include <string.h>
#include <Kernel/Device/Serial.hpp>
#include <Kernel/Debug/kdebugf.hpp>

//  FIXME: Only support COM0 for now

void Serial::_serial_irq_handler(PtraceRegs*) {

}

bool Serial::probe(Serial::Port port) {
	register_write(port, Register::IrqEn, 0x0);     //  Disable IRQ
	register_write(port, Register::IrqId, 0xc7);    //  Clear FIFO

	//  Enable loopback mode
	register_write(port, Register::ModemControl, 0x1E);

	//  Clear any pending data
	for(unsigned i = 0; i < 10; ++i)
		(void)register_read(port, Register::Data);

	const uint8 magic = 0xDA;
	register_write(port, Register::Data, magic);
	const uint8 returned = register_read(port, Register::Data);

	return magic == returned;
}

void Serial::initialize(Serial::Port port) {
	//  Set speed - 115200
	register_write(port, Register::LineControl, 0x80);  //  DLAB on
	register_write(port, Register::Data, 0x01);         //  Divisor 1
	register_write(port, Register::IrqEn, 0x00);
	register_write(port, Register::LineControl, 0x03);  //  8N1, DLAB off

	register_write(port, Register::IrqId, 0xc7);        //  FIFO enabled and cleared
	register_write(port, Register::ModemControl, 0x0f); //  Normal operation mode
	register_write(port, Register::IrqEn, 0x0B);        //  No interrupts, RTS/DTS
	IRQDispatcher::register_microtask(4, _serial_irq_handler);

	//  Clear any pending data
	for(unsigned i = 0; i < 10; ++i)
		(void)register_read(port, Register::Data);

	kdebugf("[Serial] Initialized COM%i, speed 115200, 8N1, IRQ 4\n", (unsigned)port);
}

void Serial::init() {
	const bool valid = probe(Serial::Port::COM0);
	if(valid) {
		initialize(Serial::Port::COM0);
	}
}

uint16 Serial::io_port(Serial::Port port) {
	static constexpr const uint16 io_address_for_port[4] = { 0x3F8, 0x2F8, 0x3E8, 0x2E8 };
	return io_address_for_port[static_cast<size_t>(port)%4];
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
