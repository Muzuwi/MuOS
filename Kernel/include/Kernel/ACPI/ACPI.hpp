#pragma once
#include <Kernel/Memory/Ptr.hpp>
#include <Kernel/KOptional.hpp>
#include <Kernel/SystemTypes.hpp>

struct RSDPDescriptor {
	char m_signature[8];
	uint8_t m_checksum;
	char m_oemid[6];
	uint8_t m_revision;
	uint32_t m_rsdt_addr;
} __attribute__ ((packed));

struct ACPISDTHeader {
	char m_signature[4];
	uint32_t m_length;
	uint8_t m_revision;
	uint8_t m_checksum;
	char m_oemid[6];
	char m_oemtableid[8];
	uint32_t m_oem_revision;
	uint32_t m_creator_id;
	uint32_t m_creator_revision;
} __attribute__ ((packed));


namespace ACPI {
	static const uint64_t rsdp_signature = 0x2052545020445352;
	static const uint32_t table_apic_sig = 0x43495041;

	void parse_tables();
	KOptional<PhysPtr<ACPISDTHeader>> find_table(uint32_t signature);
}