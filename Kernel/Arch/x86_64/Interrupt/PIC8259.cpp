#include "PIC8259.hpp"
#include <Arch/x86_64/PortIO.hpp>
#include <Core/Error/Error.hpp>
#include <Core/IRQ/Controller.hpp>
#include <Core/IRQ/IRQ.hpp>
#include <Core/Log/Logger.hpp>
#include <SystemTypes.hpp>

CREATE_LOGGER("x86_64::pic8259", core::log::LogLevel::Debug);

/*  Initializes the PIC8259 combo
 *
 *  Assumes two PIC8259s are present to be used for
 *  servicing interrupts.
 */
static void pic8259_init() {
	Ports::out(PIC8259_PORT_MCMD, 0x11);
	Ports::out(PIC8259_PORT_SCMD, 0x11);
	//  remap PIC IRQs starting from 0x20
	//  primary: 0x20..0x27
	//  secondary: 0x28..0x2f
	Ports::out(PIC8259_PORT_MDATA, 0x20);
	Ports::out(PIC8259_PORT_SDATA, 0x28);
	//  ensure they are chained, IRQ2 is used
	//  as a chain IRQ between the chips
	Ports::out(PIC8259_PORT_MDATA, 4);
	Ports::out(PIC8259_PORT_SDATA, 2);
	Ports::out(PIC8259_PORT_MDATA, ICW4_8086);
	Ports::out(PIC8259_PORT_SDATA, ICW4_8086);
	//  configure all interrupt lines as masked by default
	Ports::out(PIC8259_PORT_MDATA, 0xFF);
	Ports::out(PIC8259_PORT_SDATA, 0xFF);
}

/*  Shut down the PIC8259 combo
 *
 *  As a result, no interrupts will be serviced by the chips.
 *  This simply masks all interrupts.
 */
__attribute__((unused)) static void pic8259_shutdown() {
	Ports::out(PIC8259_PORT_MDATA, 0xff);
	Ports::out(PIC8259_PORT_SDATA, 0xff);
}

/*	Reads the interrupt mask registers from both PICs
 */
static uint16 pic8259_read_mask() {
	const uint8 low = Ports::in(PIC8259_PORT_MDATA);
	const uint8 high = Ports::in(PIC8259_PORT_SDATA);
	return static_cast<uint16>(high) << 8u | low;
}

/*	Configure masking state for the given IRQ line.
 */
static void pic8259_set_line_masked(uint8 id, bool masked) {
	const uint16 port = id < 8 ? PIC8259_PORT_MDATA : PIC8259_PORT_SDATA;
	const uint8 current = Ports::in(port);
	uint8 new_value;
	if(masked) {
		new_value = current | (1 << (id % 8));
	} else {
		new_value = current & ~(1 << (id % 8));
	}
	Ports::out(port, new_value);
}

static uint16_t pic8259_read_irq_reg(uint8 ocw3) {
	//  enable special mask mode
	Ports::out(PIC8259_PORT_MCMD, ocw3 | 0b01100000u);
	Ports::out(PIC8259_PORT_SCMD, ocw3 | 0b01100000u);
	const uint16 retval = (Ports::in(PIC8259_PORT_SCMD) << 8) | Ports::in(PIC8259_PORT_MCMD);
	//  disable special mask mode
	Ports::out(PIC8259_PORT_MCMD, 0b01001000);
	Ports::out(PIC8259_PORT_SCMD, 0b01001000);
	return retval;
}

/*	Reads the in-service register from both PICs
 */
static uint16 pic8259_read_isr() {
	return pic8259_read_irq_reg(0x0B);
}

/*	Reads the IRQ request register from both PICs
 */
__attribute__((unused)) static uint16 pic8259_read_irr() {
	return pic8259_read_irq_reg(0x0A);
}

static void pic8259_irq_ack(uint8 id) {
	const auto isr = pic8259_read_isr();
	//  Just a regular IRQ - acknowledge it and move on.
	if(isr & (1 << id)) {
		if(id >= 8) {
			//  ack main interrupt on child
			Ports::out(PIC8259_PORT_SCMD, 0x60 | (id & 0x07));
			//  ack chain interrupt on parent
			Ports::out(PIC8259_PORT_MCMD, 0x60 | (0x2));
		} else {
			Ports::out(PIC8259_PORT_MCMD, 0x60 | (id & 0x07));
		}
		return;
	}

	log.debug("Spurious IRQ: {}", id);
	//  This was a spurious IRQ. If it came from the
	//  slave PIC then we need to acknowledge the master
	//  chip as it is unaware that it was a spurious IRQ.
	if(id >= 8) {
		Ports::out(PIC8259_PORT_MCMD, 0x60 | (0x2));
	}
}

struct PicIrqController : public core::irq::IrqController {
	constexpr PicIrqController()
	    : core::irq::IrqController(32, 16) {}

	void acknowledge_irq(core::irq::IrqId id) override {
		id = id - this->base;
		pic8259_irq_ack(id);
	}

	bool irq_is_masked(core::irq::IrqId id) override {
		id = id - this->base;
		return pic8259_read_mask() & (1 << id);
	}

	void irq_mask(core::irq::IrqId id) override {
		id = id - this->base;
		pic8259_set_line_masked(id, true);
	}

	void irq_unmask(core::irq::IrqId id) override {
		id = id - this->base;
		pic8259_set_line_masked(id, false);
	}
};

static PicIrqController s_controller {};

void x86_64::pic8259_init() {
	::pic8259_init();
	if(const auto err = core::irq::register_controller(&s_controller); err != core::Error::Ok) {
		log.error("Failed to register PIC8259 IRQ controller! ({})", err);
	}
}
