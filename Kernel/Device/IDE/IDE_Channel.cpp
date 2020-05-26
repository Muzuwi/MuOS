#include <Kernel/Device/IDE/IDE_Channel.hpp>
#include <Kernel/ksleep.hpp>
#include <Arch/i386/PortIO.hpp>
#include <Kernel/Debug/kpanic.hpp>

/*	===========================================================================
 *								IDE Channel class
 *  ===========================================================================
 */

IDE_Channel::~IDE_Channel() {
	//  If there are any drives left when the channel is destroyed,
	//  we did something wrong
	assert(m_drives.size() == 0);
}

IDE_Channel::IDE_Channel() {
	m_base_io = 0;
	m_base_ctrl = 0;
	m_nIEN = false;
	m_bmide = 0;
}

IDE_Channel::IDE_Channel(PCI_Device& ide_controller, IDE_Channel_Type type) {
	m_nIEN = false;
	m_bmide = ide_controller.getConfigRegister(0x20);

#define isDefault(a) (a == 0x0 || a == 0x1)

	if(type == IDE_Channel_Type::Primary) {
		m_base_io = ide_controller.getConfigRegister(0x10);
		m_base_ctrl = ide_controller.getConfigRegister(0x14);

		m_base_io = isDefault(m_base_io) ? BASE_BAR0_PORT : m_base_io;
		m_base_ctrl = isDefault(m_base_ctrl) ? BASE_BAR1_PORT : m_base_ctrl;
	} else {
		m_base_io = ide_controller.getConfigRegister(0x18);
		m_base_ctrl = ide_controller.getConfigRegister(0x1c);

		m_base_io = isDefault(m_base_io) ? BASE_BAR2_PORT : m_base_io;
		m_base_ctrl = isDefault(m_base_ctrl) ? BASE_BAR3_PORT : m_base_ctrl;
	}

	kdebugf("[ide] Initialized %s IDE channel with bars %x, %x\n", type == IDE_Channel_Type::Primary ? "primary" : "secondary", m_base_io, m_base_ctrl);
}

/*
*	Returns the IO port address for the given register
 */
uint16_t IDE_Channel::get_register_port(ATA_REG reg) {
	switch(reg) {
		case ATA_REG::Data:
		case ATA_REG::Error: // Features:
		case ATA_REG::SecCount0:
		case ATA_REG::LBA0:
		case ATA_REG::LBA1:
		case ATA_REG::LBA2:
		case ATA_REG::HDDevSel:
		case ATA_REG::Command: // Status:
			return m_base_io + (uint8_t)reg;

		case ATA_REG::SecCount1:
		case ATA_REG::LBA3:
		case ATA_REG::LBA4:
		case ATA_REG::LBA5:
			return m_base_io + (uint8_t)reg - 0x6;

		case ATA_REG::Control: // AltStatus:
		case ATA_REG::DevAddress:
			return m_base_ctrl + (uint8_t)reg - 0xA;
	}
	kpanic();
}


/*
 *	Writes data to the specified channel register
 */
void IDE_Channel::write_register(ATA_REG reg, uint8_t data) {
	out(get_register_port(reg), data);
}


/*
 *	Reads the specified channel register and returns the value
 */
uint8_t IDE_Channel::read_register(ATA_REG reg) {
	return in(get_register_port(reg));
}

/*
 *	Checks whether a specified status field is set
 */
bool IDE_Channel::read_status(ATA_STATUS status){
	return (read_register(ATA_REG::Status) & (uint8_t)status);
}

/*
 *	Writes a command to the command register
 */
void IDE_Channel::write_command(uint8_t command) {
	write_register(ATA_REG::Command, command);
}

void IDE_Channel::write_command(ATA_CMD command) {
	write_register(ATA_REG::Command, (uint8_t)command);
}


ATA_STATUS IDE_Channel::poll(bool request_check) {
	for(size_t i = 0; i < 4; i++) read_register(ATA_REG::AltStatus);

	while(read_status(ATA_STATUS::Busy))
		;

	if(request_check) {
		uint8_t status = read_register(ATA_REG::Status);
		if(status & (uint8_t)ATA_STATUS::Error)				return ATA_STATUS::Error;
		else if(status & (uint8_t)ATA_STATUS::WriteFault)	return ATA_STATUS::WriteFault;
		else if(status & (uint8_t)ATA_STATUS::RequestReady) return ATA_STATUS::RequestReady;
	}

	return ATA_STATUS::Ready;
}

/*
 *	Reads a specified amount of dwords from a register into a buffer
 */
void IDE_Channel::read_buffer(ATA_REG reg, uint32_t* buffer, size_t count) {
	this->poll(true);
	for(size_t i = 0; i < count; i++) {
		buffer[i] = ind(get_register_port(reg));
		if(this->poll(false) != ATA_STATUS::Ready) {
			kerrorf("[ide] Error during read buffer operation!\n");
		}
	}
}

/*
 *	Writes a specified amount of dwords from a buffer into an IDE register
 */
void IDE_Channel::write_buffer(ATA_REG reg, uint32_t* buffer, size_t count) {
	this->poll(true);
	for(size_t i = 0; i < count; i++) {
		outd(get_register_port(reg), buffer[i]);
		if(this->poll(false) != ATA_STATUS::Ready) {
			kerrorf("[ide] Error during write buffer operation!\n");
		}
	}
}

void IDE_Channel::drive_select(IDE_Drive_Type type, AddressingType addressing, uint8_t head) {
	uint8_t byte = (type == IDE_Drive_Type::Master) ? 0xA0 : 0xB0;
	switch (addressing) {
		case AddressingType::LBA24:
		case AddressingType::LBA48:
			byte |= (1 << 6);
		default:
			break;
	}

	byte |= (head & 0xF);


	this->write_register(ATA_REG::HDDevSel, byte);
	ksleep(1);
}


/*
 *	Finds all drives on the current channel and returns them in a vector
 */
gen::vector<IDE_Drive*> IDE_Channel::find_drives() {
	for(size_t drive_index = 0; drive_index < 2; drive_index++) {
		this->drive_select(!drive_index ? IDE_Drive_Type::Master : IDE_Drive_Type::Slave);

		this->write_command(ATA_CMD::Identify);
		ksleep(1);

		if(this->read_register(ATA_REG::Status) == 0) continue;

		bool noATA = false;
		while(true) {
			uint8_t status = this->read_register(ATA_REG::Status);

			//  ATAPI devices fail with an error on ATA Identify command
			if(status & (uint8_t)ATA_STATUS::Error) {
				noATA = true;
				break;
			}
			if(!(status & (uint8_t)ATA_STATUS::Busy) && (status & (uint8_t)ATA_STATUS::RequestReady)){
				break;
			}
		}

		ATA_TYPE type = ATA_TYPE::ATA;

		//  Check if ATAPI
		if(noATA) {
			uint8_t lba1 = this->read_register(ATA_REG::LBA1),
			        lba2 = this->read_register(ATA_REG::LBA2);
			if(lba1 == 0x14 && lba2 == 0xEB) {
				type = ATA_TYPE::ATAPI;
			} else if (lba1 == 0x69 && lba2 == 0x96) {
				type = ATA_TYPE::ATAPI;
			} else {
				continue;
			}
			this->write_command(ATA_CMD::IdentifyPacket);
			ksleep(1);
		}

		uint32_t config_space[128];
		this->read_buffer(ATA_REG::Data, config_space, 128);

		IDE_Drive* drive = new IDE_Drive(*this, !drive_index ? IDE_Drive_Type::Master : IDE_Drive_Type::Slave, type, config_space);
		m_drives.push_back(drive);
	}

	return m_drives;
}

/*
 *	Some IDE controllers report 0xffff bars when probed, presumably to maintain
 *	compatibility with legacy software?
 *	This checks whether this channel's bars are valid
 */
bool IDE_Channel::valid() {
	return m_bmide != 0xFFFF &&
	       m_base_io != 0xFFFF &&
	       m_base_ctrl != 0xFFFF;
}

gen::vector<IDE_Drive*> IDE_Channel::getDrives() {
	return m_drives;
}
