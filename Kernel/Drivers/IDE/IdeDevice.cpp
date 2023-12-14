#include <Arch/x86_64/PortIO.hpp>
#include <Core/Error/Error.hpp>
#include <Debug/klogf.hpp>
#include <Drivers/IDE/IDE.hpp>
#include <LibGeneric/LockGuard.hpp>
#include <SystemTypes.hpp>

using namespace driver::ide;

//  FIXME: Driver hangs when a request for partial out-of-bound
//  sectors is sent (so for example, read of 2 sectors starting at
//  `m_sectors-1`.

/**
 * 	Try initializing the specified drive.
 * 	This performs detection of the drive's addressing mode and attempts to read
 * 	the sector count.
 */
bool IdeDevice::initialize() {
	const bool rc = identify();
	if(!rc) {
		return false;
	}
	klogf("[driver::ide] ide: successfully read identity space\n");

	for(auto i = 0; i < 32; i++) {
		klogf("[driver::ide] ");
		for(auto j = 0; j < 16; j++) {
			klogf("{x}", m_identity[i * 16 + j]);
		}
		klogf("\n");
	}

	auto supported_addressing = detect_addressing();
	if(supported_addressing & AddressingSupportSet::FeatureLBA28) {
		klogf("[driver::ide]  supports LBA28\n");
		m_mode = AddressingMode::LBA28;
	}
	if(supported_addressing & AddressingSupportSet::FeatureLBA48) {
		klogf("[driver::ide]  supports LBA48\n");
		m_mode = AddressingMode::LBA48;
	}
	auto sectors = read_sector_count();
	klogf("[driver::ide] Using mode: {}, sectors: {}\n", (m_mode == AddressingMode::LBA28 ? "LBA28" : "LBA48"),
	      sectors);
	m_sectors = sectors;

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

	if(wait_until_ready() & AtaStatus::Error) {
		//  An error occured
		return false;
	}

	m_identity.resize(512);
	read_sector(m_identity.begin());

	return true;
}

AddressingSupportSet IdeDevice::detect_addressing() {
	uint64 ret { 0 };

	const uint16 word = read_identity_u16(83 * 2);
	if(word & (1u << 10u)) {
		ret |= AddressingSupportSet::FeatureLBA48;
	}

	const uint32 lba28_sectors = read_identity_u32(60 * 2);
	if(lba28_sectors > 0) {
		ret |= AddressingSupportSet::FeatureLBA28;
	}

	return static_cast<AddressingSupportSet>(ret);
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
			kerrorf("[driver::ide] Invalid addressing mode: {}\n", static_cast<size_t>(m_mode));
			return 0;
		}
	}
}

void IdeDevice::register_write(AtaRegister reg, uint8 byte) const {
	Ports::out(m_disk_base_port + static_cast<uint16>(reg), byte);
}

void IdeDevice::register_write(AtaControlRegister reg, uint8 byte) const {
	Ports::out(m_device_base_port + static_cast<uint16>(reg), byte);
}

uint8 IdeDevice::register_read(AtaRegister reg) const {
	return Ports::in(m_disk_base_port + static_cast<uint16>(reg));
}

uint8 IdeDevice::register_read(AtaControlRegister reg) const {
	return Ports::in(m_device_base_port + static_cast<uint16>(reg));
}

void IdeDevice::send_command(AtaCommand command) const {
	register_write(AtaRegister::Command, static_cast<uint8>(command));
}

uint16 IdeDevice::register_read_data16() const {
	return Ports::inw(m_disk_base_port + static_cast<uint16>(AtaRegister::Data));
}

void IdeDevice::register_write_data16(uint16 data) const {
	Ports::outw(m_disk_base_port + static_cast<uint16>(AtaRegister::Data), data);
}

uint8 IdeDevice::wait_until_ready() {
	uint8 status;
	do {
		status = register_read(AtaRegister::Status);
	} while(!(status & AtaStatus::DataRequestReady) && !(status & AtaStatus::Error));
	return status;
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

/**
 * Perform an access to the drive starting at sector `base_sector`,
 * spanning `count` sectors. Depending on `dir`, this can be either
 * a read or write.
 */
core::Error IdeDevice::access(uint64 base_sector, uint16 count, uint8* buf, size_t buf_len, Direction dir) {
	gen::LockGuard lock { m_lock };

	//  Sanity check input buffers
	const auto required_size = count * sector_size();
	if(buf_len < required_size) {
		kerrorf("[drivers::ide] Input buffer of size {x} is too small for read operation of {} sectors ({x})\n",
		        buf_len, count, required_size);
		return core::Error::InvalidArgument;
	}

	switch(m_mode) {
		case AddressingMode::LBA28: {
			return access_lba28(base_sector, count, buf, dir);
		}
		case AddressingMode::LBA48: {
			return access_lba48(base_sector, count, buf, dir);
		}
		default: {
			return core::Error::IOFail;
		}
	}
}

core::Error IdeDevice::access_lba28(uint32, uint16, uint8*, Direction) {
	//  FIXME: Implement this
	return core::Error::IOFail;
}

core::Error IdeDevice::access_lba48(uint64 base_sector, uint16 count, uint8* buf, Direction dir) {
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
		if(wait_until_ready() & AtaStatus::Error) {
			//  FIXME: Retry the access N amount of times
			const auto current_lba = base_sector + done;
			kerrorf("[driver::ide] Error bit set, disk access to LBA {x} failed!\n", current_lba);
			return core::Error::IOFail;
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

	return core::Error::Ok;
}
