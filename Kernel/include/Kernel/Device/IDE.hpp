#pragma once
#include <stdint.h>
#include <stddef.h>
#include <Kernel/Debug/kdebugf.hpp>

#define IDE_CONTROLLER_BASE_CLASS 1
#define IDE_CONTROLLER_SUBCLASS 1

#define BASE_BAR0_PORT 0x1f0
#define BASE_BAR1_PORT 0x3f6
#define BASE_BAR2_PORT 0x170
#define BASE_BAR3_PORT 0x376
#define BASE_BAR4_PORT 0

//  Status fields
enum class ATA_STATUS {
	Busy		 = 0x80,
	Ready		 = 0x40,
	WriteFault	 = 0x20,
	SeekComplete = 0x10,
	RequestReady = 0x08,
	Corrected	 = 0x04,
	Index		 = 0x02,
	Error		 = 0x01
};

//  Error fields
enum class ATA_ERROR {
	BadBlock		= 0x80,
	Uncorrectable	= 0x40,
	MediaChanged	= 0x20,
	NoID			= 0x10,
	MediaChangeReq	= 0x08,
	Abort			= 0x04,
	NoTrack0		= 0x02,
	NoAddressMark	= 0x01,
};

//  Commands
enum class ATA_CMD {
	ReadPIO			= 0x20,
	ReadPIOExt		= 0x24,
	ReadDMA			= 0xC8,
	ReadDMAExt		= 0x25,
	WritePIO		= 0x30,
	WritePIOExt		= 0x34,
	WriteDMA		= 0xCA,
	WriteDMAExt		= 0x35,
	CacheFlush		= 0xe7,
	CacheFlushExt	= 0xea,
	Packet			= 0xa0,
	IdentifyPacket	= 0xa1,
	Identify		= 0xec,
};

//  Channels
enum class IDE_Channel_Type {
	Primary = 0,
	Secondary = 1
};

#define ATAPI_CMD_READ			  0xA8
#define ATAPI_CMD_EJECT			  0x1B
#define ATA_IDENT_DEVICETYPE   0
#define ATA_IDENT_CYLINDERS    2
#define ATA_IDENT_HEADS        6
#define ATA_IDENT_SECTORS      12
#define ATA_IDENT_SERIAL       20
#define ATA_IDENT_MODEL        54
#define ATA_IDENT_CAPABILITIES 98
#define ATA_IDENT_FIELDVALID   106
#define ATA_IDENT_MAX_LBA      120
#define ATA_IDENT_COMMANDSETS  164
#define ATA_IDENT_MAX_LBA_EXT  200

enum class ATA_TYPE {
	ATA = 0x0,
	ATAPI = 0x1
};

enum class IDE_Drive_Type {
	Master = 0x0,
	Slave = 0x1
};

enum class AddressingType {
	CHS = 0,
	LBA24 = 1,
	LBA48 = 2
};


enum class ATA_REG {
	Data = 0,
	Error = 0x1,
	Features = 0x1,
	SecCount0 = 0x2,
	LBA0 = 0x3,
	LBA1 = 0x4,
	LBA2 = 0x5,
	HDDevSel = 0x6,
	Command = 0x7,
	Status = 0x7,
	SecCount1 = 0x8,
	LBA3 = 0x9,
	LBA4 = 0xA,
	LBA5 = 0xB,
	Control = 0xC,
	AltStatus = 0xC,
	DevAddress = 0xD,
};

#define IDE_CHANNEL_PRIMARY 0
#define IDE_CHANNEL_SECONDARY 1


class SectorCache {
	void* m_cache;
	size_t m_sector_count;
	size_t m_sector_size;
	uint32_t m_sector_base;
public:
	SectorCache(){
		m_cache = nullptr;
		m_sector_base = 0;
		m_sector_count = 0;
		m_sector_size = 0;
	}

	SectorCache(void* cache, size_t sector_count, size_t sector_base, size_t sector_size) {
		m_cache = cache;
		m_sector_base = sector_base;
		m_sector_count = sector_count;
		m_sector_size = sector_size;
//		kdebugf("[ide_drive] New sector cache, sector count %i, base: %x, sector size: %x\n", sector_count, sector_base, sector_size);
	}

	SectorCache(const SectorCache& v)
	: m_cache(v.m_cache), m_sector_base(v.m_sector_base), m_sector_size(v.m_sector_size), m_sector_count(v.m_sector_count){ }

	SectorCache& operator=(const SectorCache& v) {
		if(&v != this) {
			m_cache = v.m_cache;
			m_sector_base = v.m_sector_base;
			m_sector_count = v.m_sector_count;
			m_sector_size = v.m_sector_size;
		}

		return *this;
	}

	bool present() {
		return m_cache != nullptr;
	}

	void* getCache() {
		return m_cache;
	}

	size_t getCachedSectorCount() {
		return m_sector_count;
	}

	uint32_t getSectorAddress() {
		return m_sector_base;
	}

	bool contains(uint32_t address) {
		return address >= m_sector_base*m_sector_size && (address < (m_sector_base + m_sector_count)*m_sector_size);
	}

	void* get_cache_at_addr(uint32_t addr) {
		if(addr < m_sector_base*m_sector_size)
			return nullptr;
		else
			return reinterpret_cast<void*>((uint8_t*)m_cache + (addr - m_sector_base*m_sector_size));
	}

	void invalidate() {
		delete[] m_cache;
		m_sector_count = 0;
		m_sector_size = 0;
		m_sector_base = 0;
	}

};



namespace IDE {
    void check_devices();
};
