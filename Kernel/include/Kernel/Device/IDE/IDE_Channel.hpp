#pragma once
#include <stdint.h>
#include <Kernel/Device/PCI.hpp>
#include <Kernel/Device/IDE.hpp>
#include <Kernel/Device/IDE/IDE_Drive.hpp>
#include <LibGeneric/Vector.hpp>

class IDE_Channel {
	IDE_Channel_Type m_type;
	uint16_t m_base_ctrl;
	uint16_t m_base_io;
	uint16_t m_bmide;
	bool m_nIEN;
	uint16_t get_register_port(ATA_REG reg);
	gen::vector<IDE_Drive*> m_drives;
public:
	~IDE_Channel();
	IDE_Channel();
	IDE_Channel(PCI_Device& controller, IDE_Channel_Type type);

	void write_register(ATA_REG reg, uint8_t data);
	void write_command(ATA_CMD command);
	void write_command(uint8_t command);

	uint8_t read_register(ATA_REG reg);
	bool read_status(ATA_STATUS status);

	void read_buffer(ATA_REG reg, uint32_t* buffer, size_t count);
	void write_buffer(ATA_REG reg, uint32_t* buffer, size_t count);

	void drive_select(IDE_Drive_Type type, AddressingType addressing=AddressingType::CHS, uint8_t heads=0);
	ATA_STATUS poll(bool=false);

	bool valid();

	gen::vector<IDE_Drive*> find_drives();
	gen::vector<IDE_Drive*> getDrives();
};
