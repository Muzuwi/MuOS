#include <Arch/x86_64/ACPI.hpp>
#include <Core/Log/Logger.hpp>
#include <LibGeneric/List.hpp>
#include <Memory/Ptr.hpp>
#include <Structs/KOptional.hpp>

CREATE_LOGGER("x86_64::acpi", core::log::LogLevel::Debug);

//  FIXME: ACPI/Validate checksums for tables

static PhysPtr<ACPISDTHeader> s_rsdt;
static gen::List<PhysPtr<ACPISDTHeader>> s_tables;

KOptional<PhysPtr<RSDPDescriptor>> find_rsdp() {
	for(unsigned long i = 0x000E0000; i < 0x000FFFFF; i += 16) {
		PhysAddr addr { (void*)i };
		uint64_t v = *addr.as<uint64_t>();

		if(v == ACPI::rsdp_signature) {
			return { PhysPtr { (RSDPDescriptor*)i } };
		}
	}

	return {};
}

void ACPI::parse_tables() {
	auto maybe_rsdp = find_rsdp();
	if(!maybe_rsdp.has_value()) {
		log.error("No RSDP pointer found!");
		ENSURE_NOT_REACHED();
	}

	auto const& rsdp = *maybe_rsdp.unwrap();
	log.debug("RSDP: checksum={}, revision={}, OEMID='{}{}{}{}{}{}', RSDT={x}", rsdp.m_checksum, rsdp.m_revision,
	          rsdp.m_oemid[0], rsdp.m_oemid[1], rsdp.m_oemid[2], rsdp.m_oemid[3], rsdp.m_oemid[4], rsdp.m_oemid[5],
	          rsdp.m_rsdt_addr);
	s_rsdt = PhysPtr<ACPISDTHeader> { (ACPISDTHeader*)static_cast<uintptr_t>(rsdp.m_rsdt_addr) };

	auto const& rsdt = *s_rsdt;
	log.debug("RSDT: checksum={x}, length={}, revision={}, OEMID='{}{}{}{}{}{}'", rsdt.m_checksum, rsdt.m_length,
	          rsdt.m_revision, rsdt.m_oemid[0], rsdt.m_oemid[1], rsdt.m_oemid[2], rsdt.m_oemid[3], rsdt.m_oemid[4],
	          rsdt.m_oemid[5]);

	unsigned entries = (rsdt.m_length - sizeof(ACPISDTHeader)) / 4;
	auto entries_base = PhysPtr<uint32_t> { (uint32_t*)(s_rsdt + 1).get() };

	for(unsigned j = 0; j < entries; ++j) {
		uint32_t table_address = *(entries_base + j);
		auto table_ptr = PhysPtr<ACPISDTHeader> { (ACPISDTHeader*)static_cast<uintptr_t>(table_address) };
		auto const& table_header = *table_ptr;

		log.debug("Table {}{}{}{}: length={}, revision={}, checksum={x}, OEMID='{}{}{}{}{}{}'",
		          table_header.m_signature[0], table_header.m_signature[1], table_header.m_signature[2],
		          table_header.m_signature[3], table_header.m_length, table_header.m_revision, table_header.m_revision,
		          table_header.m_oemid[0], table_header.m_oemid[1], table_header.m_oemid[2], table_header.m_oemid[3],
		          table_header.m_oemid[4], table_header.m_oemid[5]);

		s_tables.push_back(table_ptr);
	}

	log.debug("Found {} tables", s_tables.size());
}

KOptional<PhysPtr<ACPISDTHeader>> ACPI::find_table(uint32_t signature) {
	for(auto& ptr : s_tables) {
		auto const& table_header = *ptr;
		if(*reinterpret_cast<uint32_t const*>(&table_header.m_signature[0]) == signature) {
			return { ptr };
		}
	}
	return {};
}
