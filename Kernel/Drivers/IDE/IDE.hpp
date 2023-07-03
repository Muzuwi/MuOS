#pragma once
#include <Core/Error/Error.hpp>
#include <LibGeneric/Vector.hpp>
#include <stddef.h>
#include <SystemTypes.hpp>

namespace driver::ide {
	enum AtaStatus {
		Busy = 0x80,
		DriveReady = 0x40,
		DriveWriteFault = 0x20,
		DriveSeekComplete = 0x10,
		DataRequestReady = 0x08,
		CorrectedData = 0x04,
		Index = 0x02,
		Error = 0x01
	};

	enum class AtaCommand {
		ReadPortIO = 0x20,
		ReadPortIOExt = 0x24,
		ReadDMA = 0xC8,
		ReadDMAExt = 0x25,
		WritePortIO = 0x30,
		WritePortIOExt = 0x34,
		WriteDMA = 0xCA,
		WriteDMAExt = 0x35,
		CacheFlush = 0xE7,
		CacheFlushExt = 0xEA,
		Packet = 0xA0,
		IdentifyPacket = 0xA1,
		Identify = 0xEC
	};

	enum IdentifySpace {
		DeviceType = 0,
		Cylinders = 2,
		Heads = 6,
		Sectors = 12,
		Serial = 20,
		Model = 54,
		Capabilities = 98,
		FieldValid = 106,
		MaxLBA = 120,
		CommandSets = 164,
		MaxLBAExt = 200
	};

	enum class AtaRegister {
		Data = 0x0,
		ErrorReg = 0x1,
		Features = 0x1,
		SectorCount0 = 0x2,
		LBA0 = 0x3,
		LBA1 = 0x4,
		LBA2 = 0x5,
		DevSelect = 0x6,
		Command = 0x7,
		Status = 0x7,
		SectorCount1 = 0x8,
		LBA3 = 0x9,
		LBA4 = 0xA,
		LBA5 = 0xB,
	};
	enum class AtaControlRegister {
		Control = 0x0,
		AltStatus = 0x0,
		DevAddress = 0x1
	};

	enum class Direction {
		Read = 0x0,
		Write = 0x1
	};

	enum class DriveSelect {
		Master = 0x0,
		Slave = 0x1
	};

	enum AddressingSupportSet {
		FeatureNull = 0,
		FeatureLBA28 = 0x1,
		FeatureLBA48 = 0x2
	};
	enum class AddressingMode {
		LBA28,
		LBA48
	};

	core::Error init();

	class IdeDevice;
}

class driver::ide::IdeDevice {
public:
	constexpr IdeDevice(uint16 disk_control, uint16 device_control) noexcept
	    : m_disk_base_port(disk_control)
	    , m_device_base_port(device_control) {}

	[[nodiscard]] constexpr uint32 sector_size() const { return m_sector_size; }
	[[nodiscard]] constexpr uint64 sectors() const { return m_sectors; }
	bool initialize();
private:
	const uint16 m_disk_base_port;
	const uint16 m_device_base_port;
	const driver::ide::DriveSelect m_which { driver::ide::DriveSelect::Master };
	const uint32 m_sector_size { 512 };
	gen::Vector<uint8> m_identity {};
	driver::ide::AddressingMode m_mode {};
	uint64 m_sectors { 0 };

	constexpr uint8 read_identity_u8(size_t offset) const { return m_identity[offset]; }

	constexpr uint16 read_identity_u16(size_t offset) const {
		return static_cast<uint16>(read_identity_u8(offset)) |
		       (static_cast<uint16>(read_identity_u8(offset + 1)) << 8u);
	}

	constexpr uint32 read_identity_u32(size_t offset) const {
		return static_cast<uint32>(read_identity_u16(offset)) |
		       (static_cast<uint32>(read_identity_u16(offset + 2)) << 16u);
	}

	constexpr uint64 read_identity_u64(size_t offset) const {
		return static_cast<uint64>(read_identity_u32(offset)) |
		       (static_cast<uint64>(read_identity_u32(offset + 4)) << 32u);
	}

	void read_sector(uint8* buf);
	void write_sector(uint8* buf);

	uint16 register_read_data16() const;
	void register_write_data16(uint16 data) const;
	void send_command(driver::ide::AtaCommand command) const;
	void register_write(driver::ide::AtaControlRegister reg, uint8 byte) const;
	void register_write(driver::ide::AtaRegister reg, uint8 byte) const;
	uint8 register_read(driver::ide::AtaControlRegister reg) const;
	uint8 register_read(driver::ide::AtaRegister reg) const;
	bool identify();
	driver::ide::AddressingSupportSet detect_addressing();
	uint64 read_sector_count();
	uint8 wait_until_ready();

	bool access(uint64 base_sector, uint16 count, uint8* buf, driver::ide::Direction dir);
	bool access_lba28(uint32 base_sector, uint16 count, uint8* buf, driver::ide::Direction dir);
	bool access_lba48(uint64 base_sector, uint16 count, uint8* buf, driver::ide::Direction dir);
};
