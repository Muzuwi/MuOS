#pragma once
#include <SystemTypes.hpp>

static constexpr uint16 PIC8259_PORT_MCMD = 0x0020;
static constexpr uint16 PIC8259_PORT_MDATA = 0x0021;
static constexpr uint16 PIC8259_PORT_SCMD = 0x00A0;
static constexpr uint16 PIC8259_PORT_SDATA = 0x00A1;

/* 8086/88 (MCS-80/85) mode */
static constexpr uint8 ICW4_8086 = 0x01;

namespace x86_64 {
	void pic8259_init();
}
