#pragma once
#include <stdint.h>
#include <stddef.h>
#include <Kernel/Device/IDE.hpp>
#include <Kernel/Filesystem/VirtualBlockDevice.hpp>

class IDE_Channel;

class IDE_Drive : public VirtualBlockDevice {
	IDE_Channel *m_channel;
	IDE_Drive_Type m_drive_type;
	ATA_TYPE m_ata_type;
	AddressingType m_supports_addressing;

	uint32_t m_sector_size;
	uint32_t m_commands;
	uint32_t m_size;
	uint16_t m_signature;
	uint16_t m_capabilities;
	char m_model[41];


public:
	bool ide_access(bool read, uint32_t LBA, uint8_t sectors, uint32_t* buffer);

	void select_drive(AddressingType addressing=AddressingType::CHS, uint8_t heads=0);
	IDE_Drive();
	IDE_Drive(IDE_Channel&, IDE_Drive_Type, ATA_TYPE, uint32_t*);
	size_t getSectorSize();

	uint64_t getFreeBytes();
	uint64_t getTotalBytes();

	IOResult read_block(lba_t address, uintptr_t *dst, size_t size) override;
	IOResult write_block(lba_t address, uintptr_t *dst, size_t size) override;
	size_t getBlockSize() override;
};
