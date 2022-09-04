#include "PciDevice.hpp"
#include "Arch/x86_64/PortIO.hpp"
#include "PCI.hpp"

uint32 PciDevice::read_config(size_t offset) const {
	const auto address = (uint32)((((uint32)m_bus << 16) | ((uint32)m_device << 11) | ((uint32)m_function << 8) |
	                               ((uint32)offset & (uint32)~0x3)) |
	                              ((uint32)0x80000000));
	Ports::outd(PCI::config_address_port, address);
	return Ports::ind(PCI::config_data_port);
}

void PciDevice::write_config(size_t offset, uint32 value) {
	const auto address = (uint32)((((uint32)m_bus << 16) | ((uint32)m_device << 11) | ((uint32)m_function << 8) |
	                               ((uint32)offset & (uint32)~0x3)) |
	                              ((uint32)0x80000000));
	Ports::outd(PCI::config_address_port, address);
	Ports::outd(PCI::config_data_port, value);
}
