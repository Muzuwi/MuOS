#pragma once
#include <Memory/Ptr.hpp>
#include <Structs/KOptional.hpp>
#include <SystemTypes.hpp>

struct RSDPDescriptor {
	char m_signature[8];
	uint8 m_checksum;
	char m_oemid[6];
	uint8 m_revision;
	uint32 m_rsdt_addr;
} __attribute__((packed));

struct ACPISDTHeader {
	char m_signature[4];
	uint32 m_length;
	uint8 m_revision;
	uint8 m_checksum;
	char m_oemid[6];
	char m_oemtableid[8];
	uint32 m_oem_revision;
	uint32 m_creator_id;
	uint32 m_creator_revision;
} __attribute__((packed));

class ACPI {
	static ACPI s_instance;

	PhysPtr<ACPISDTHeader> m_rsdt;

	ACPI() = default;
	static KOptional<PhysPtr<RSDPDescriptor>> find_rsdp();
	static bool verify_checksum(PhysPtr<ACPISDTHeader> table);
public:
	static void initialize();
	static KOptional<PhysPtr<ACPISDTHeader>> find_table(uint32 signature);

	static constexpr uint64 rsdp_signature = 0x2052545020445352;
	static constexpr uint32 table_apic_sig = 0x43495041;
};
