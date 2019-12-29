#include <stdint.h>
#include <string.h>
#include <Kernel/Device/IDE/IDE_Drive.hpp>
#include <Kernel/Device/IDE/IDE_Channel.hpp>
#include <Kernel/Debug/kpanic.hpp>

/*	===========================================================================
 *								IDE Drive class
 *  ===========================================================================
 */

IDE_Drive::IDE_Drive() {
	m_channel = nullptr;
	m_size = 0;
	m_capabilities = 0;
	m_commands = 0;
}

IDE_Drive::IDE_Drive(IDE_Channel& channel, IDE_Drive_Type drive_type, ATA_TYPE ata_type, uint32_t* conf_space) {
#define byte(a) (*((uint8_t*)((uint32_t)conf_space + a)))
#define word(a) (*((uint16_t*)((uint32_t)conf_space + a)))
#define dword(a) (*((uint32_t*)((uint32_t)conf_space + a)))
	kdebugf("[ide] Drive is an %s drive\n", ata_type == ATA_TYPE::ATA ? "ATA"
	                                                                  : "ATAPI");
	m_channel = &channel;
	m_drive_type = drive_type;
	m_ata_type = ata_type;
	m_commands = dword(ATA_IDENT_COMMANDSETS);
	m_capabilities = word(ATA_IDENT_CAPABILITIES);
	m_sector_size = 512;

	if(m_commands & (1 << 26)) {
		m_size = dword(ATA_IDENT_MAX_LBA_EXT);
	} else {
		m_size = dword(ATA_IDENT_MAX_LBA);
	}

	for(size_t k = 0; k < 40; k += 2) {
		m_model[k] = byte(ATA_IDENT_MODEL + k + 1);
		m_model[k+1] = byte(ATA_IDENT_MODEL + k);
	}
	m_model[40] = '\0';

	kdebugf("[ide] Found %s drive '%s'\n", m_drive_type == IDE_Drive_Type::Master ? "master" : "slave", &m_model[0]);
	kdebugf("[ide] Drive size: %i\n", m_size);
}

/*
 *	Select this drive in the main controller
 */
void IDE_Drive::select_drive(AddressingType addressing, uint8_t head_number) {
	this->m_channel->drive_select(m_drive_type, addressing, head_number);
}

bool IDE_Drive::ide_access(bool read, uint32_t LBA, uint8_t sectors, uint32_t* buffer) {
	AddressingType LBA_type;
	ATA_CMD command;
	bool DMA = false;

	uint8_t LBA_IO[6];

	uint8_t sects, cyls, heads;

	if(LBA >= 0x10000000) {
		LBA_type = AddressingType::LBA48;
		LBA_IO[0] = (LBA & 0x000000FF);
		LBA_IO[1] = (LBA & 0x0000FF00) >> 8;
		LBA_IO[2] = (LBA & 0x00FF0000) >> 16;
		LBA_IO[3] = (LBA & 0xFF000000) >> 24;
		LBA_IO[4] = 0;
		LBA_IO[5] = 0;
		heads = 0;
	} else if(m_capabilities & 0x200) {
		LBA_type = AddressingType::LBA24;
		LBA_IO[0] = (uint8_t)(LBA & 0x000000FF);
		LBA_IO[1] = (uint8_t)((LBA & 0x0000FF00) >> 8);
		LBA_IO[2] = (uint8_t)((LBA & 0x00FF0000) >> 16);
		LBA_IO[3] = 0;
		LBA_IO[4] = 0;
		LBA_IO[5] = 0;
		heads = (LBA & 0x0F000000) >> 24;
	} else {
		LBA_type = AddressingType::CHS;
		sects = (LBA % 63) + 1;
		cyls = (LBA + 1 - sects) / (16 * 63);
		LBA_IO[0] = sects;
		LBA_IO[1] = cyls & 0xFF;
		LBA_IO[2] = (cyls >> 8) & 0xFF;
		LBA_IO[3] = 0;
		LBA_IO[4] = 0;
		LBA_IO[5] = 0;
		heads = ((LBA + 1 - sects) % (16*63)) / (63);
	}

	while(this->m_channel->read_status(ATA_STATUS::Busy))
		;

	this->select_drive(LBA_type, heads);

	if(LBA_type == AddressingType::LBA48) {
		this->m_channel->write_register(ATA_REG::SecCount1, 0);
		this->m_channel->write_register(ATA_REG::LBA3, LBA_IO[3]);
		this->m_channel->write_register(ATA_REG::LBA4, LBA_IO[4]);
		this->m_channel->write_register(ATA_REG::LBA5, LBA_IO[5]);
	}

	this->m_channel->write_register(ATA_REG::SecCount0, sectors);
	this->m_channel->write_register(ATA_REG::LBA0, LBA_IO[0]);
	this->m_channel->write_register(ATA_REG::LBA1, LBA_IO[1]);
	this->m_channel->write_register(ATA_REG::LBA2, LBA_IO[2]);


	if(read) {
		if(DMA) {
			if(LBA_type == AddressingType::CHS)		   command = ATA_CMD::ReadDMA;
			else if(LBA_type == AddressingType::LBA24) command = ATA_CMD::ReadDMA;
			else if(LBA_type == AddressingType::LBA48) command = ATA_CMD::ReadDMAExt;
		} else {
			if(LBA_type == AddressingType::CHS)		   command = ATA_CMD::ReadPIO;
			else if(LBA_type == AddressingType::LBA24) command = ATA_CMD::ReadPIO;
			else if(LBA_type == AddressingType::LBA48) command = ATA_CMD::ReadPIOExt;
		}
	} else {
		if(DMA) {
			if(LBA_type == AddressingType::CHS)		   command = ATA_CMD::WriteDMA;
			else if(LBA_type == AddressingType::LBA24) command = ATA_CMD::WriteDMA;
			else if(LBA_type == AddressingType::LBA48) command = ATA_CMD::WriteDMAExt;
		} else {
			if(LBA_type == AddressingType::CHS)		   command = ATA_CMD::WritePIO;
			else if(LBA_type == AddressingType::LBA24) command = ATA_CMD::WritePIO;
			else if(LBA_type == AddressingType::LBA48) command = ATA_CMD::WritePIOExt;
		}
	}

	this->m_channel->write_command(command);
	this->m_channel->poll();

	uint32_t words = sectors * (m_sector_size / sizeof(uint32_t));
	if(read) {
		kdebugf("[ide] Requested %i sectors, %i dwords\n", sectors, words);
		this->m_channel->read_buffer(ATA_REG::Data, buffer, words);
	} else {
		this->m_channel->write_buffer(ATA_REG::Data, buffer, words);
		this->m_channel->write_command(ATA_CMD::CacheFlush);
		this->m_channel->poll();
	}

	return true;
}

size_t IDE_Drive::getSectorSize() {
	return m_sector_size;
}


uint64_t IDE_Drive::getFreeBytes() {
	assert(false);
}

uint64_t IDE_Drive::getTotalBytes() {
	assert(false);
}

/*
 *	VirtualDrive overloaded functions
 */

IOResult IDE_Drive::read_block(lba_t address, uintptr_t *destination_buffer, size_t size) {
	size_t sectors = (size / m_sector_size) + (size % m_sector_size) ? 1 : 0;
	if(!this->ide_access(true, address, sectors, destination_buffer)) {
		kerrorf("[ide_drive] Read LBA %x failed\n");
	}
	return 1;
}

IOResult IDE_Drive::write_block(lba_t address, uintptr_t *destination_buffer, size_t size) {
	assert(false);
}

size_t IDE_Drive::getBlockSize() {
	return m_sector_size;
}
