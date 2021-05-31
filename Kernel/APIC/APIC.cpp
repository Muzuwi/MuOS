#include <string.h>
#include <LibGeneric/List.hpp>
#include <Kernel/APIC/APIC.hpp>
#include <Kernel/ACPI/ACPI.hpp>
#include <Kernel/Debug/kdebugf.hpp>
#include <Kernel/Memory/Ptr.hpp>
#include <Kernel/ksleep.hpp>

static PhysAddr s_local_apic_base;
static gen::List<uint8_t> s_ap_ids;

uint32_t APIC::lapic_read(LAPICReg reg) {
	return *(s_local_apic_base + static_cast<size_t>(reg)).as<uint32_t>();
}

void APIC::lapic_write(LAPICReg reg, uint32_t val) {
	*(s_local_apic_base + static_cast<size_t>(reg)).as<uint32_t>() = val;
}

void APIC::discover() {
	APIC::find_local_base();

	uint32_t la_id = lapic_read(LAPICReg::APICID);
	uint32_t la_version = lapic_read(LAPICReg::APICVer);
	kdebugf("[APIC] BSP Local APIC ID=%i, version=%i\n", la_id, la_version);

	PhysAddr a{(void*)0x20000};
	const uint8_t shellcode[] =
			{ 0x8C, 0xC8, 0x8E, 0xD8, 0x8E, 0xC0, 0x8E, 0xE0, 0x8E, 0xE8, 0x8E, 0xD0, 0xB0, 0x01, 0xA2, 0xF0, 0x0F, 0x00, 0x00, 0xF4, 0xEB, 0xFD }
			;

	for(uint8_t id : s_ap_ids) {
		if(la_id == id) continue;
		kdebugf("[APIC] Bringing up node %i...", id);

		for(unsigned i = 0; i < sizeof(shellcode); ++i) {
			*(a+i).as<uint8_t>() = shellcode[i];
		}
		*(a+0xff0).as<uint8_t volatile>() = 0x0;

		uint32_t ap_id = (uint32_t)id << 24u;
		uint32_t lo = 0x520;    //  Init IPI
		uint32_t lo1 = 0x620;   //  Startup IPI

		lapic_write(LAPICReg::ICRHi, ap_id);
		lapic_write(LAPICReg::ICRLow, lo);
		ksleep(10);

		lapic_write(LAPICReg::ICRLow, lo1);

		while (*(a+0xff0).as<uint8_t volatile>() == 0)
			;

		kdebugf("up/parked\n");
	}

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
	kdebugf("[APIC] Local APIC at %x, flags=%x\n", *(madt + 0x24).as<uint32_t>(), *(madt + 0x28).as<uint32_t>());

	unsigned offset = 0x2c;
	while(offset < madt_header.m_length) {
		auto entry_type = *(madt + offset).as<uint8_t>();
		auto rec_len = *(madt + offset + 1).as<uint8_t>();

		switch (entry_type) {
			case 0: {
				auto acpi_processor_id = *(madt + offset + 2).as<uint8_t>();
				auto apic_id = *(madt + offset + 3).as<uint8_t>();
				auto flags = *(madt + offset + 4).as<uint32_t>();
				s_ap_ids.push_back(apic_id);
				kdebugf("[APIC] Entry: Processor %i, APIC_ID=%i, Flags=%x\n", acpi_processor_id, apic_id, flags);
				break;
			}
			case 1: {
				auto ioapic_id = *(madt + offset + 2).as<uint8_t>();
				auto ioapic_addr = *(madt + offset + 4).as<uint32_t>();
				auto global_irq_base = *(madt + offset + 8).as<uint32_t>();
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

	uint32_t local_apic = *(madt + 0x24).as<uint32_t>();
	s_local_apic_base = PhysAddr{(void*)local_apic};
}
