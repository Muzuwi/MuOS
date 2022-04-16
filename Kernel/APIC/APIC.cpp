#include <ACPI/ACPI.hpp>
#include <APIC/APIC.hpp>
#include <Arch/x86_64/PortIO.hpp>
#include <Debug/klogf.hpp>
#include <LibGeneric/StaticVector.hpp>
#include <Kernel/ksleep.hpp>
#include <Memory/Ptr.hpp>
#include <SMP/SMP.hpp>

APIC APIC::s_instance {};

/*
 *  Reads a specified LAPIC register
 */
uint32 APIC::lapic_read(LAPICReg reg) {
	return *(s_instance.m_local_apic_base + static_cast<size_t>(reg)).as<uint32 volatile>();
}

/*
 *  Writes a value to the specified LAPIC register
 */
void APIC::lapic_write(LAPICReg reg, uint32 val) {
	*(s_instance.m_local_apic_base + static_cast<size_t>(reg)).as<uint32 volatile>() = val;
}

/*
 *  Reads a specified I/O APIC's register
 */
uint32 APIC::ioapic_read(PhysAddr ioapic, uint32 reg) {
	auto ptr = ioapic.as<uint32 volatile>();
	ptr[0] = (reg & 0xFFu);
	return ptr[4];
}

/*
 *  Writes a value to the specified I/O APIC's register
 */
void APIC::ioapic_write(PhysAddr ioapic, uint32 reg, uint32 value) {
	auto ptr = ioapic.as<uint32 volatile>();
	ptr[0] = (reg & 0xFFu);
	ptr[4] = value;
}

/*
 *  Initializes the APIC subsystem. Finds the proper LAPIC base address, and sets up receiving
 *  PIC interrupts on the BSP, along with calibrating the LAPIC timer to be used instead of the PIT.
 */
void APIC::initialize() {
	APIC::find_local_base();

	uint32 la_id = lapic_read(LAPICReg::APICID);
	uint32 la_version = lapic_read(LAPICReg::APICVer);
	klogf("[APIC] BSP Local APIC ID={}, version={}\n", la_id, la_version);

	s_instance.m_bootstrap_ap = la_id;

	APIC::redirect_isa_to_bsp();

	//  Disable the PIC
	Ports::out(0xa1, 0xff);
	Ports::out(0x21, 0xff);
	//  Enable ExtINTs from PIC on the BSP
	lapic_write(LAPICReg::LVTLINT0, 0x08700);
	//  Initialize LAPIC and calibrate APIC timer
	APIC::initialize_this_ap();
}

/*
 *  Finds the MADT table and parses the structures to find runnable AP's on the system
 */
void APIC::find_local_base() {
	auto maybe_madt = ACPI::find_table(ACPI::table_apic_sig);
	if(!maybe_madt.has_value()) {
		kerrorf("[APIC] MADT/APIC table not present!\n");
		ASSERT_NOT_REACHED();
	}
	PhysAddr madt = PhysAddr { (maybe_madt.unwrap()).get() };
	s_instance.m_madt = madt;

	auto const& madt_header = *maybe_madt.unwrap();
	klogf("[APIC] MADT/APIC table length={}\n", madt_header.m_length);
	klogf("[APIC] Local APIC at {x}, flags={x}\n", *(madt + 0x24).as<uint32>(), *(madt + 0x28).as<uint32>());

	unsigned offset = 0x2c;
	while(offset < madt_header.m_length) {
		auto entry_type = *(madt + offset).as<uint8>();
		auto rec_len = *(madt + offset + 1).as<uint8>();

		switch(entry_type) {
			case 0: {
				auto acpi_processor_id = *(madt + offset + 2).as<uint8>();
				auto apic_id = *(madt + offset + 3).as<uint8>();
				auto flags = *(madt + offset + 4).as<uint32>();
				//  Check if online capable AP and isn't already running
				if(!(flags & 1) && !(flags & 2)) {
					break;
				}

				s_instance.m_ap_ids.push_back(apic_id);
				klogf("[APIC] Processor {}, APIC_ID={}, Flags={x}\n", acpi_processor_id, apic_id, flags);
				break;
			}
			case 1: {
				auto ioapic_id = *(madt + offset + 2).as<uint8>();
				auto ioapic_addr = *(madt + offset + 4).as<uint32>();
				auto global_irq_base = *(madt + offset + 8).as<uint32>();
				klogf("[APIC] I/O APIC ID={}, addr={x}, irq_base={}\n", ioapic_id, ioapic_addr, global_irq_base);
				break;
			}
			case 2: {
				auto bus_source = *(madt + offset + 2).as<uint8>();
				auto irq_source = *(madt + offset + 3).as<uint8>();
				auto global_system_irq = *(madt + offset + 4).as<uint32>();
				auto flags = *(madt + offset + 8).as<uint16>();
				klogf("[APIC] IO/APIC Override: sourcebus={}, sourceirq={}, irq={}, flags={x}\n", bus_source,
				      irq_source, global_system_irq, flags);
				break;
			}
			case 3: {
				auto nmi_source = *(madt + offset + 2).as<uint8>();
				auto flags = *(madt + offset + 4).as<uint16>();
				auto global_system_irq = *(madt + offset + 6).as<uint32>();
				klogf("[APIC] IO/APIC NMI: source={} flags={}, irq={}\n", nmi_source, flags, global_system_irq);
				break;
			}
			case 4: {
				auto proc_id = *(madt + offset + 2).as<uint8>();
				auto flags = *(madt + offset + 3).as<uint16>();
				auto lint = *(madt + offset + 5).as<uint8>();
				klogf("[APIC] Local APIC NMI: processor={x}, flags={x}, LINT={}\n", proc_id, flags, lint);
				break;
			}
			case 5: {
				auto addr = *(madt + offset + 4).as<uint64>();
				klogf("[APIC] Local APIC address override: {x}", addr);
				break;
			}
			default: break;
		}

		offset += rec_len;
	}

	uint32 local_apic = *(madt + 0x24).as<uint32>();
	s_instance.m_local_apic_base = PhysAddr { (void*)static_cast<uintptr_t>(local_apic) };
}

/*
 *  Redirect a specific global IRQ to a local IRQ
 */
bool APIC::redirect_irq(uint8 global_irq_vector, uint8 local_irq_vector, DeliveryMode mode, IrqPolarity polarity,
                        IrqTrigger trigger_mode, uint8 destination) {
	auto const& madt_header = *s_instance.m_madt.as<ACPISDTHeader>();

	unsigned offset = 0x2c;
	while(offset < madt_header.m_length) {
		auto entry_type = *(s_instance.m_madt + offset).as<uint8>();
		auto rec_len = *(s_instance.m_madt + offset + 1).as<uint8>();

		if(entry_type == 1) {
			auto ioapic_addr = *(s_instance.m_madt + offset + 4).as<uint32>();
			auto global_irq_base = *(s_instance.m_madt + offset + 8).as<uint32>();
			if(global_irq_base > global_irq_vector) {
				offset += rec_len;
				continue;
			}
			auto ioapic = PhysAddr { (void*)ioapic_addr };
			auto count = (ioapic_read(ioapic, 0x01) >> 16u) + 1;

			if(global_irq_vector >= global_irq_base + count) {
				offset += rec_len;
				continue;
			}

			const auto which = global_irq_vector - global_irq_base;

			//  Redirect it
			const auto h = destination << 24u;
			const auto l = local_irq_vector | (static_cast<uint32>(mode) << 8u) |
			               (static_cast<uint32>(polarity) << 13u) | (static_cast<uint32>(trigger_mode) << 15u);
			ioapic_write(ioapic, 0x10 + which * 2, l);
			ioapic_write(ioapic, 0x11 + which * 2, h);

			return true;
		}
		offset += rec_len;
	}

	return false;
}

/*
 *  Redirects global IRQs that have their own override entry in the MADT to their original source IRQs on the BSP.
 */
void APIC::redirect_isa_to_bsp() {
	auto const& madt_header = *s_instance.m_madt.as<ACPISDTHeader>();

	unsigned offset = 0x2c;
	while(offset < madt_header.m_length) {
		auto entry_type = *(s_instance.m_madt + offset).as<uint8>();
		auto rec_len = *(s_instance.m_madt + offset + 1).as<uint8>();

		if(entry_type == 2) {
			auto irq_source = *(s_instance.m_madt + offset + 3).as<uint8>();
			auto global_system_irq = *(s_instance.m_madt + offset + 4).as<uint32>();
			auto flags = *(s_instance.m_madt + offset + 8).as<uint16>();

			const auto polarity = (flags & 2) ? IrqPolarity::ActiveLow : IrqPolarity::ActiveHigh;
			const auto trigger = (flags & 8) ? IrqTrigger::Level : IrqTrigger::Edge;
			kassert(APIC::redirect_irq(global_system_irq, 32 + irq_source, DeliveryMode::Normal, polarity, trigger,
			                           APIC::ap_bootstrap_id()));
			klogf("[APIC] Redirected global_irq{{{}}} -> local_irq{{{}}} on the BSP\n", global_system_irq,
			      32 + irq_source);
		}

		offset += rec_len;
	}
}

/*
 *  Reinitializes this AP's LAPIC, and calibrates the APIC timer using the PIT
 */
void APIC::initialize_this_ap() {
	//  Disable timer, performance, error int's
	lapic_write(LAPICReg::LVTTimer, 0x10000);
	lapic_write(LAPICReg::LVTPerformance, 0x10000);
	lapic_write(LAPICReg::LVTError, 0x10000);
	//  Enable NMI handling
	lapic_write(LAPICReg::LVTLINT1, 0x00400);
	//  Set SIVR
	lapic_write(LAPICReg::SIV, 0x1FF);
	lapic_write(LAPICReg::TPR, 0x0);

	//  Recalibrate the APIC timer
	APIC::initialize_apic_timer();
}

/*
 *  Recalibrate the APIC timer using the PIT
 */
void APIC::initialize_apic_timer() {
	//  Set up the timer, divider 16, vector 0xF0
	lapic_write(LAPICReg::TIMDIVCNT, 0x3);
	lapic_write(LAPICReg::LVTTimer, 0u | (0xF0u));
	lapic_write(LAPICReg::TIMINITCNT, 0xFFFFFFFF);

	//  Sleep for 10ms
	ksleep(10);

	//  Mask the interrupt for the APIC timer
	lapic_write(LAPICReg::LVTTimer, (1u << 16u) | (0xF0u));

	const uint32 ticks_in_10ms = 0xFFFFFFFF - lapic_read(LAPICReg::TIMCURCNT);

	klogf("[APIC(VID={})] APIC Timer running at {} Hz, cal. using PIT\n", this_cpu().vid(), ticks_in_10ms * 100 * 16);

	//  Reenable the timer, periodic mode, IRQ vector 0xF0
	IRQDispatcher::register_microtask(0xD0, APIC::lapic_timer_irq_handler);
	lapic_write(LAPICReg::TIMDIVCNT, 0x3);
	lapic_write(LAPICReg::LVTTimer, (0b01u << 17u) | (0xF0u));
	lapic_write(LAPICReg::TIMINITCNT, ticks_in_10ms);
}

/*
 *  IRQ microtask for the LAPIC timer
 */
DEFINE_MICROTASK(APIC::lapic_timer_irq_handler) {
	this_cpu().scheduler().tick();
}
