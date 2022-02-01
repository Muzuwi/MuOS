#pragma once
#include <Kernel/SystemTypes.hpp>
#include <Kernel/KOptional.hpp>
#include <LibGeneric/List.hpp>

enum class LAPICReg : unsigned {
	APICID = 0x020,
	APICVer = 0x030,

	EOI = 0x0B0,
	SIV = 0x0F0,

	ESR     = 0x280,
	LVTCMCI = 0x2F0,

	ICRLow = 0x300,
	ICRHi  = 0x310,

	LVTTimer = 0x320,
	LVTThermal = 0x330,
	LVTPerformance = 0x340,
	LVTLINT0 = 0x350,
	LVTLINT1 = 0x360,
	LVTError = 0x370
};

namespace APIC {
	void find_local_base();
	void discover();

	uint32 lapic_read(LAPICReg);
	void lapic_write(LAPICReg, uint32);

	gen::List<uint8> const& ap_list();
	uint8 ap_bootstrap_id();
}