#include <Arch/x86_64/PortIO.hpp>
#include <Debug/klogf.hpp>
#include <Drivers/IDE/IDE.hpp>

bool IdeDevice::initialize() {
	const bool rc = identify();
	if(!rc) {
		return false;
	}
	klogf("ide: successfully read identity space\n");

	for(auto i = 0; i < 32; i++) {
		for(auto j = 0; j < 16; j++) {
			klogf("{x}", m_identity[i * 16 + j]);
		}
		klogf("\n");
	}

	auto supported_addressing = detect_addressing();
	if(supported_addressing & AddressingSupportSet::FeatureLBA28) {
		klogf("  supports LBA28\n");
		m_mode = AddressingMode::LBA28;
	}
	if(supported_addressing & AddressingSupportSet::FeatureLBA48) {
		klogf("  supports LBA48\n");
		m_mode = AddressingMode::LBA48;
	}
	auto sectors = read_sector_count();
	klogf("Using mode: {}, sectors: {}\n", (m_mode == AddressingMode::LBA28 ? "LBA28" : "LBA48"), sectors);

	auto* first_sector = reinterpret_cast<uint8*>(KHeap::instance().chunk_alloc(m_sector_size * 4));
	auto v = access(2, 4, first_sector, Direction::Read);
	if(!v) {
		kerrorf("Failed accessing second sector\n");
	} else {
		for(auto i = 0; i < 4 * m_sector_size / 16; i++) {
			for(auto j = 0; j < 16; ++j) {
				klogf("{x} ", first_sector[i * 16 + j]);
			}
			klogf("\n");
		}

		first_sector[0] = 0xde;
		first_sector[1] = 0xad;
		first_sector[2] = 0xba;
		first_sector[3] = 0xbe;
		first_sector[512] = 0xf0;
		first_sector[513] = 0x0b;

		v = access(2, 4, first_sector, Direction::Write);
		if(!v) {
			kerrorf("failed writing modified sectors\n");
		}
	}
	KHeap::instance().chunk_free(first_sector);

	return true;
}

bool IdeDevice::identify() {
	const uint8 select_byte = m_which == DriveSelect::Master ? static_cast<uint8>(0xA0) : static_cast<uint8>(0xB0);
	register_write(AtaRegister::DevSelect, select_byte);
	register_write(AtaRegister::SectorCount0, 0);
	register_write(AtaRegister::LBA0, 0);
	register_write(AtaRegister::LBA1, 0);
	register_write(AtaRegister::LBA2, 0);
	send_command(AtaCommand::Identify);

	if(register_read(AtaRegister::Status) == 0) {
		return false;
	}

	while(register_read(AtaRegister::Status) & AtaStatus::Busy)
		;
	if(register_read(AtaRegister::LBA1) != 0 || register_read(AtaRegister::LBA2) != 0) {
		//  Not ATA disks
		return false;
	}

	uint8 status;
	do {
		status = register_read(AtaRegister::Status);
	} while(!(status & AtaStatus::DataRequestReady) && !(status & AtaStatus::Error));

	if(status & AtaStatus::Error) {
		//  An error occured
		return false;
	}

	m_identity.resize(512);
	read_sector(m_identity.begin());

	return true;
}

IdeDevice::AddressingSupportSet IdeDevice::detect_addressing() {
	uint64 ret { 0 };

	const uint16 word = read_identity_u16(83 * 2);
	if(word & (1u << 10u)) {
		ret |= AddressingSupportSet::FeatureLBA48;
	}

	const uint32 lba28_sectors = read_identity_u32(60 * 2);
	if(lba28_sectors > 0) {
		ret |= AddressingSupportSet::FeatureLBA28;
	}

	return static_cast<IdeDevice::AddressingSupportSet>(ret);
}

uint64 IdeDevice::read_sector_count() {
	switch(m_mode) {
		case AddressingMode::LBA28: {
			return read_identity_u32(60 * 2);
		}
		case AddressingMode::LBA48: {
			return read_identity_u64(100 * 2);
		}
		default: {
			ASSERT_NOT_REACHED();
		}
	}
}

void IdeDevice::register_write(IdeDevice::AtaRegister reg, uint8 byte) const {
	Ports::out(m_disk_base_port + static_cast<uint16>(reg), byte);
}

void IdeDevice::register_write(IdeDevice::AtaControlRegister reg, uint8 byte) const {
	Ports::out(m_device_base_port + static_cast<uint16>(reg), byte);
}

uint8 IdeDevice::register_read(IdeDevice::AtaRegister reg) const {
	return Ports::in(m_disk_base_port + static_cast<uint16>(reg));
}

uint8 IdeDevice::register_read(IdeDevice::AtaControlRegister reg) const {
	return Ports::in(m_device_base_port + static_cast<uint16>(reg));
}

void IdeDevice::send_command(IdeDevice::AtaCommand command) const {
	register_write(AtaRegister::Command, static_cast<uint8>(command));
}

uint16 IdeDevice::register_read_data16() const {
	return Ports::inw(m_disk_base_port + static_cast<uint16>(IdeDevice::AtaRegister::Data));
}

void IdeDevice::register_write_data16(uint16 data) const {
	Ports::outw(m_disk_base_port + static_cast<uint16>(IdeDevice::AtaRegister::Data), data);
}

void IdeDevice::read_sector(uint8* buf) {
	for(auto i = 0u; i < m_sector_size / 2; ++i) {
		const uint16 data = register_read_data16();
		*buf = data & 0xFF;
		buf++;
		*buf = data >> 8u;
		buf++;
	}
}

void IdeDevice::write_sector(uint8* buf) {
	for(auto i = 0u; i < m_sector_size / 2; ++i) {
		const auto low = *buf;
		buf++;
		const auto high = *buf;
		buf++;

		const uint16 data = static_cast<uint16>(low) | (static_cast<uint16>(high) << 8u);
		register_write_data16(data);
	}
}

bool IdeDevice::access(uint64 base_sector, uint16 count, uint8* buf, IdeDevice::Direction dir) {
	switch(m_mode) {
		case AddressingMode::LBA28: {
			return access_lba28(base_sector, count, buf, dir);
		}
		case AddressingMode::LBA48: {
			return access_lba48(base_sector, count, buf, dir);
		}
		default: {
			ASSERT_NOT_REACHED();
		}
	}
}

bool IdeDevice::access_lba28(uint32 base_sector, uint16 count, uint8* buf, IdeDevice::Direction dir) {
	ASSERT_NOT_REACHED();
	return false;
}

bool IdeDevice::access_lba48(uint64 base_sector, uint16 count, uint8* buf, IdeDevice::Direction dir) {
	register_write(AtaRegister::DevSelect, (m_which == DriveSelect::Master ? 0x40 : 0x50));

	//  Write LBA
	register_write(AtaRegister::SectorCount0, (count >> 8u) & 0xFFu);
	register_write(AtaRegister::LBA0, (base_sector >> 24u) & 0xFFu);
	register_write(AtaRegister::LBA1, (base_sector >> 32u) & 0xFFu);
	register_write(AtaRegister::LBA2, (base_sector >> 40u) & 0xFFu);
	register_write(AtaRegister::SectorCount0, count & 0xFFu);
	register_write(AtaRegister::LBA0, base_sector & 0xFFu);
	register_write(AtaRegister::LBA1, (base_sector >> 8u) & 0xFFu);
	register_write(AtaRegister::LBA2, (base_sector >> 16u) & 0xFFu);

	send_command(dir == Direction::Read ? AtaCommand::ReadPortIOExt : AtaCommand::WritePortIOExt);

	auto done = 0;
	do {
		uint8 status;
		do {
			status = register_read(AtaRegister::Status);
		} while(!(status & AtaStatus::DataRequestReady) && !(status & AtaStatus::Error));
		if(status & AtaStatus::Error) {
			kerrorf("ide: disk access lba {x} failure\n");
			return false;
		}

		if(dir == Direction::Read) {
			read_sector(buf);
		} else {
			write_sector(buf);
		}
		send_command(AtaCommand::CacheFlushExt);

		buf += m_sector_size;
		done++;
	} while(done < count);

	return true;
}
