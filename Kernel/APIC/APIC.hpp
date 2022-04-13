#pragma once
#include <Interrupt/IRQDispatcher.hpp>
#include <LibGeneric/List.hpp>
#include <Memory/Ptr.hpp>
#include <Structs/KOptional.hpp>
#include <SystemTypes.hpp>

enum class LAPICReg : unsigned {
	APICID = 0x020,
	APICVer = 0x030,
	TPR = 0x80,

	EOI = 0x0B0,
	DFR = 0x0E0,
	SIV = 0x0F0,

	ESR = 0x280,
	LVTCMCI = 0x2F0,

	ICRLow = 0x300,
	ICRHi = 0x310,

	LVTTimer = 0x320,
	LVTThermal = 0x330,
	LVTPerformance = 0x340,
	LVTLINT0 = 0x350,
	LVTLINT1 = 0x360,
	LVTError = 0x370,
	TIMINITCNT = 0x380,
	TIMCURCNT = 0x390,
	TIMDIVCNT = 0x3e0,
};

enum class DeliveryMode {
	Normal = 0,
	LowPriority = 1,
	SMI = 2,
	NMI = 4,
	INIT = 5,
	EXT = 7
};

enum class IrqPolarity {
	ActiveHigh = 0,
	ActiveLow = 1
};

enum class IrqTrigger {
	Edge = 0,
	Level = 1
};

class APIC {
	static APIC s_instance;
	PhysAddr m_local_apic_base;
	PhysAddr m_madt;
	gen::List<uint8> m_ap_ids;
	uint8 m_bootstrap_ap;

	static DEFINE_MICROTASK(lapic_timer_irq_handler);
	static uint32 ioapic_read(PhysAddr ioapic, uint32 reg);
	static void ioapic_write(PhysAddr ioapic, uint32 reg, uint32 value);
	static void initialize_apic_timer();
	static void find_local_base();
	static void redirect_isa_to_bsp();
	APIC() = default;
public:
	static bool redirect_irq(uint8 global_irq_vector, uint8 local_irq_vector, DeliveryMode, IrqPolarity, IrqTrigger,
	                         uint8 destination);

	static uint32 lapic_read(LAPICReg);
	static void lapic_write(LAPICReg, uint32);

	/*
	 *  Acknowledges a LAPIC interrupt on the local CPU.
	 */
	static void eoi() { lapic_write(LAPICReg::EOI, 0x0); }

	/*
	 *  List of all AP's visible and flagged as runnable in the MADT
	 *  FIXME: Refactor
	 */
	static gen::List<uint8> const& ap_list() { return s_instance.m_ap_ids; }

	/*
	 *  APIC ID of the BSP
	 *  FIXME: Refactor
	 */
	static uint8 ap_bootstrap_id() { return s_instance.m_bootstrap_ap; }

	static void initialize();
	static void initialize_this_ap();
};