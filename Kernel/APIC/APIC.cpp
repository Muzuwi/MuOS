#include <ACPI/ACPI.hpp>
#include <APIC/APIC.hpp>
#include <Debug/klogf.hpp>
#include <LibGeneric/StaticVector.hpp>
#include <Memory/Ptr.hpp>
#include <string.h>

static PhysAddr s_local_apic_base;
static gen::StaticVector<uint8_t, 512> s_ap_ids {};
static uint8 s_bootstrap_ap;

uint32 APIC::lapic_read(LAPICReg reg) {
	return *(s_local_apic_base + static_cast<size_t>(reg)).as<uint32>();
}

void APIC::lapic_write(LAPICReg reg, uint32 val) {
	*(s_local_apic_base + static_cast<size_t>(reg)).as<uint32>() = val;
}

void APIC::discover() {
	APIC::find_local_base();

	uint32 la_id = lapic_read(LAPICReg::APICID);
	uint32 la_version = lapic_read(LAPICReg::APICVer);
	klogf("[APIC] BSP Local APIC ID={}, version={}\n", la_id, la_version);

	s_bootstrap_ap = la_id;

	/*
	PhysAddr a{(void*)0x20000};
	const uint8 shellcode[] =
	        { 0x8C, 0xC8, 0x8E, 0xD8, 0x8E, 0xC0, 0x8E, 0xE0, 0x8E, 0xE8, 0x8E, 0xD0, 0xB0, 0x01, 0xA2, 0xF0, 0x0F,
	0x00, 0x00, 0xF4, 0xEB, 0xFD }
	        ;

	for(uint8 id : s_ap_ids) {
	    if(la_id == id) continue;
	    kdebugf("[APIC] Bringing up node %i...", id);

	    for(unsigned i = 0; i < sizeof(shellcode); ++i) {
	        *(a+i).as<uint8>() = shellcode[i];
	    }
	    *(a+0xff0).as<uint8 volatile>() = 0x0;

	    uint32 ap_id = (uint32)id << 24u;
	    uint32 lo = 0x520;    //  Init IPI
	    uint32 lo1 = 0x620;   //  Startup IPI

	    lapic_write(LAPICReg::ICRHi, ap_id);
	    lapic_write(LAPICReg::ICRLow, lo);
	    ksleep(10);

	    lapic_write(LAPICReg::ICRLow, lo1);

	    while (*(a+0xff0).as<uint8 volatile>() == 0)
	        ;

	    kdebugf("up/parked\n");
	}
	 */
}

void APIC::find_local_base() {
	auto maybe_madt = ACPI::find_table(ACPI::table_apic_sig);
	if(!maybe_madt.has_value()) {
		kerrorf("[APIC] MADT/APIC table not present!\n");
		ASSERT_NOT_REACHED();
	}
	PhysAddr madt = PhysAddr { (maybe_madt.unwrap()).get() };

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

				s_ap_ids.push_back(apic_id);
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
	s_local_apic_base = PhysAddr { (void*)static_cast<uintptr_t>(local_apic) };
}

gen::StaticVector<uint8, 512> const& APIC::ap_list() {
	return s_ap_ids;
}

uint8 APIC::ap_bootstrap_id() {
	return s_bootstrap_ap;
}
