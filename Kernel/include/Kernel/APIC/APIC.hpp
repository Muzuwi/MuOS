#pragma once
#include <Kernel/KOptional.hpp>

enum class LAPICReg : unsigned {
	APICID = 0x020,
	APICVer = 0x030,

	ICRLow = 0x300,
	ICRHi  = 0x310,
};

namespace APIC {
	void find_local_base();
	void discover();

	uint32_t lapic_read(LAPICReg);
	void lapic_write(LAPICReg, uint32_t);
}