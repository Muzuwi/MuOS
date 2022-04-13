#include <ACPI/ACPI.hpp>
#include <Debug/klogf.hpp>
#include <Debug/kpanic.hpp>
#include <LibGeneric/List.hpp>
#include <Memory/Ptr.hpp>
#include <Structs/KOptional.hpp>

ACPI ACPI::s_instance {};

/*
 *  Searches memory from 0x000E0000 to 0x000FFFFF for the RSDP descriptor signature.
 *  Returns the physical pointer to the descriptor if found.
 */
KOptional<PhysPtr<RSDPDescriptor>> ACPI::find_rsdp() {
	for(unsigned long i = 0x000E0000; i < 0x000FFFFF; i += 16) {
		PhysAddr addr { (void*)i };
		uint64 v = *addr.as<uint64>();

		if(v == ACPI::rsdp_signature) {
			return { PhysPtr { (RSDPDescriptor*)i } };
		}
	}

	return {};
}

/*
 *  Verifies the checksum of the specified ACPI table.
 */
bool ACPI::verify_checksum(PhysPtr<ACPISDTHeader> table) {
	if(!table) {
		return false;
	}

	auto length = table->m_length;
	auto ptr = reinterpret_cast<uint8 const*>(table.get_mapped());
	uint8 value = 0;
	while(length) {
		value += *ptr;
		ptr++;
		length--;
	}

	return value == 0;
}

/*
 *  Initializes the ACPI subsystem. Finds the RSDP of the system and verifies the checksums of all tables.
 */
void ACPI::initialize() {
	auto maybe_rsdp = find_rsdp();
	if(!maybe_rsdp.has_value()) {
		kerrorf_static("[ACPI] No RSDP pointer found!\n");
		kpanic();
	}

	//  Helper for printing chars from signature arrays
	auto c = [](char ch) -> char {
		if(ch < 32) {
			return '.';
		}
		return ch;
	};

	auto const& rsdp = *maybe_rsdp.unwrap();
	klogf_static("[ACPI] RSDP: checksum={}, revision={}, OEMID='{}{}{}{}{}{}', RSDT={x}\n", rsdp.m_checksum,
	             rsdp.m_revision, c(rsdp.m_oemid[0]), c(rsdp.m_oemid[1]), c(rsdp.m_oemid[2]), c(rsdp.m_oemid[3]),
	             c(rsdp.m_oemid[4]), c(rsdp.m_oemid[5]), rsdp.m_rsdt_addr);
	s_instance.m_rsdt = PhysPtr<ACPISDTHeader> { (ACPISDTHeader*)static_cast<uintptr_t>(rsdp.m_rsdt_addr) };

	auto const& rsdt = *s_instance.m_rsdt;
	klogf_static("[ACPI] RSDT: checksum={x}, length={}, revision={}, OEMID='{}{}{}{}{}{}'\n", rsdt.m_checksum,
	             rsdt.m_length, rsdt.m_revision, c(rsdt.m_oemid[0]), c(rsdt.m_oemid[1]), c(rsdt.m_oemid[2]),
	             c(rsdt.m_oemid[3]), c(rsdt.m_oemid[4]), c(rsdt.m_oemid[5]));
	if(!verify_checksum(s_instance.m_rsdt)) {
		kerrorf_static("[ACPI] ACPI Error: RSDT table failed validation.\n");
		kpanic();
	}

	unsigned entries = (rsdt.m_length - sizeof(ACPISDTHeader)) / 4;
	auto entries_base = PhysPtr<uint32> { (uint32*)(s_instance.m_rsdt + 1).get() };

	for(unsigned j = 0; j < entries; ++j) {
		uint32 table_address = *(entries_base + j);
		auto table_ptr = PhysPtr<ACPISDTHeader> { (ACPISDTHeader*)static_cast<uintptr_t>(table_address) };
		auto const& table_header = *table_ptr;

		klogf_static("[ACPI] Table {}{}{}{}: length={}, revision={}, checksum={x}, OEMID='{}{}{}{}{}{}'\n",
		             table_header.m_signature[0], table_header.m_signature[1], table_header.m_signature[2],
		             table_header.m_signature[3], table_header.m_length, table_header.m_revision,
		             table_header.m_revision, c(table_header.m_oemid[0]), c(table_header.m_oemid[1]),
		             c(table_header.m_oemid[2]), c(table_header.m_oemid[3]), c(table_header.m_oemid[4]),
		             c(table_header.m_oemid[5]));
		if(!verify_checksum(table_ptr)) {
			kerrorf_static("[ACPI] ACPI Error: Table '{}{}{}{}' failed validation.\n", table_header.m_signature[0],
			               table_header.m_signature[1], table_header.m_signature[2], table_header.m_signature[3]);
			kpanic();
		}
	}

	klogf_static("[ACPI] Found {} tables\n", entries);
}

/*
 *  Attempts to find an ACPI table with the specified signature.
 *  Returns a physical pointer to the header if found.
 */
KOptional<PhysPtr<ACPISDTHeader>> ACPI::find_table(uint32 signature) {
	unsigned entries = ((*s_instance.m_rsdt).m_length - sizeof(ACPISDTHeader)) / 4;
	auto entries_base = PhysPtr<uint32> { (uint32*)(s_instance.m_rsdt + 1).get() };

	for(unsigned j = 0; j < entries; ++j) {
		uint32 table_address = *(entries_base + j);
		auto table_ptr = PhysPtr<ACPISDTHeader> { (ACPISDTHeader*)static_cast<uintptr_t>(table_address) };
		auto const& table_header = *table_ptr;
		if(*reinterpret_cast<uint32 const*>(&table_header.m_signature[0]) == signature) {
			return { table_ptr };
		}
	}

	return {};
}
