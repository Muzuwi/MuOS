#pragma once
#include <Kernel/SystemTypes.hpp>
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

	uint32 lapic_read(LAPICReg);
	void lapic_write(LAPICReg, uint32);

	gen::List<uint8> const& ap_list();
	uint8 ap_bootstrap_id();
}