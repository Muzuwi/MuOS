#include <Arch/x86_64/PCI/PCI.hpp>
#include <Arch/x86_64/PCI/PciDevice.hpp>
#include <Arch/x86_64/PortIO.hpp>
#include <Core/Error/Error.hpp>
#include <Core/IO/BlockDevice.hpp>
#include <Core/Object/Tree.hpp>
#include <Debug/klogf.hpp>
#include <Drivers/IDE/IDE.hpp>
#include <LibFormat/Format.hpp>
#include <LibGeneric/Memory.hpp>
#include <Memory/KHeap.hpp>
#include <Structs/KFunction.hpp>
#include <SystemTypes.hpp>
#include "LibGeneric/String.hpp"

using namespace driver::ide;

static gen::String format_name(uint16 ctl_port, uint16 dev_port) {
	char output[16];
	Format::format("ide@{x}:{x}", output, sizeof(output), ctl_port, dev_port);
	return gen::String { output };
}

/**
 * 	Probe the specified I/O ports for an ATA PIO drive.
 */
static void ide_probe(uint16 disk_control, uint16 device_control) {
	auto drive = IdeDevice { disk_control, device_control };
	if(!drive.initialize()) {
		kerrorf("[driver::ide] Failed initializing drive @ IO {x}, {x}\n", disk_control, device_control);
		return;
	}

	auto* alloc = reinterpret_cast<core::io::BlockDevice*>(KHeap::allocate(sizeof(core::io::BlockDevice)));
	if(!alloc) {
		kerrorf("[driver::ide] Allocating BlockDevice failed\n");
		return;
	}

	auto* object = gen::construct_at(alloc, format_name(disk_control, device_control));
	if(const auto err = core::obj::attach(object); err != core::Error::Ok) {
		kerrorf("[driver::ide] BlockDevice object attach failed, core::Error: {x}\n", static_cast<size_t>(err));
		return;
	}
}

/**
 * 	Initialize the IDE subsystem.
 * 	Perform initial probing of available ATA PIO drives and
 * 	register valid ones in the subsystem.
 */
core::Error driver::ide::init() {
	auto maybe_dev = PCI::acquire_device(
	        [](PciDevice const& device) -> bool { return device.class_() == 0x01 && device.subclass() == 0x01; });
	if(!maybe_dev.has_value()) {
		klogf("[driver::ide] detect: No PCI IDE controllers detected\n");
		return core::Error::Ok;
	}

	auto dev = maybe_dev.unwrap();
	auto prog_if = dev.prog_if();
	kerrorf("[driver::ide] PCI IDE controller with device_id={x}, vendor_id={x}\n", dev.device_id(), dev.vendor_id());
	if(prog_if & 0b0101) {
		kerrorf("[driver::ide] PCI native mode unsupported, controller incompatible\n");
		return core::Error::Ok;
	}

	uint16 primary_base = 0x1F0, primary_base_control = 0x3F6;
	uint16 secondary_base = 0x170, secondary_base_control = 0x376;
	ide_probe(primary_base, primary_base_control);
	ide_probe(secondary_base, secondary_base_control);

	return core::Error::Ok;
}

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
			ASSERT_NOT_REACHED();
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

bool IdeDevice::access(uint64 base_sector, uint16 count, uint8* buf, Direction dir) {
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

bool IdeDevice::access_lba28(uint32 base_sector, uint16 count, uint8* buf, Direction dir) {
	//  FIXME: Implement
	ASSERT_NOT_REACHED();
	return false;
}

bool IdeDevice::access_lba48(uint64 base_sector, uint16 count, uint8* buf, Direction dir) {
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
