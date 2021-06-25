#include <string.h>
#include <LibGeneric/List.hpp>
#include <Kernel/APIC/APIC.hpp>
#include <Kernel/ACPI/ACPI.hpp>
#include <Kernel/Debug/kdebugf.hpp>
#include <Kernel/Memory/Ptr.hpp>
#include <Kernel/ksleep.hpp>

static PhysAddr s_local_apic_base;
static gen::List<uint8_t> s_ap_ids;
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
	kdebugf("[APIC] BSP Local APIC ID=%i, version=%i\n", la_id, la_version);

	s_bootstrap_ap = la_id;

	/*
	PhysAddr a{(void*)0x20000};
	const uint8 shellcode[] =
			{ 0x8C, 0xC8, 0x8E, 0xD8, 0x8E, 0xC0, 0x8E, 0xE0, 0x8E, 0xE8, 0x8E, 0xD0, 0xB0, 0x01, 0xA2, 0xF0, 0x0F, 0x00, 0x00, 0xF4, 0xEB, 0xFD }
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
	PhysAddr madt = PhysAddr{(maybe_madt.unwrap()).get()};

	auto const& madt_header = *maybe_madt.unwrap();
	kdebugf("[APIC] MADT/APIC table length=%i\n", madt_header.m_length);
	kdebugf("[APIC] Local APIC at %x, flags=%x\n", *(madt + 0x24).as<uint32>(), *(madt + 0x28).as<uint32>());

	unsigned offset = 0x2c;
	while(offset < madt_header.m_length) {
		auto entry_type = *(madt + offset).as<uint8>();
		auto rec_len = *(madt + offset + 1).as<uint8>();

		switch (entry_type) {
			case 0: {
				auto acpi_processor_id = *(madt + offset + 2).as<uint8>();
				auto apic_id = *(madt + offset + 3).as<uint8>();
				auto flags = *(madt + offset + 4).as<uint32>();
				//  Check if online capable AP
				if(!(flags & 2)) {
					break;
				}

				s_ap_ids.push_back(apic_id);
				kdebugf("[APIC] Entry: Processor %i, APIC_ID=%i, Flags=%x\n", acpi_processor_id, apic_id, flags);
				break;
			}
			case 1: {
				auto ioapic_id = *(madt + offset + 2).as<uint8>();
				auto ioapic_addr = *(madt + offset + 4).as<uint32>();
				auto global_irq_base = *(madt + offset + 8).as<uint32>();
				kdebugf("[APIC] Entry: I/O APIC ID=%i, addr=%x, irq_base=%x\n", ioapic_id, ioapic_addr, global_irq_base);
				break;
			}
			case 2: {
				kdebugf("[APIC] Entry: Type 2\n");
				break;
			}
			case 4: {
				kdebugf("[APIC] Entry: Type 4\n");
				break;
			}
			case 5: {
				kdebugf("[APIC] Entry: Type 5\n");
				break;
			}
			default: break;
		}

		offset += rec_len;
	}

	uint32 local_apic = *(madt + 0x24).as<uint32>();
	s_local_apic_base = PhysAddr{(void*)local_apic};
}

gen::List<uint8> const& APIC::ap_list() {
	return s_ap_ids;
}

uint8 APIC::ap_bootstrap_id() {
	return s_bootstrap_ap;
}
