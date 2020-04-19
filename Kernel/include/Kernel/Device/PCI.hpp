#pragma once
#include <stdint.h>
#include <LibGeneric/Vector.hpp>

#define CONFIG_ADDRESS 0xCF8
#define CONFIG_DATA    0xCFC

enum PCI_FIELD16 {
	W_DEVICEID,
	W_VENDORID,
	W_STATUS,
	W_COMMAND,
};

enum PCI_FIELD8 {
	B_CLASS,
	B_SUBCLASS,
	B_IF,
	B_REVISION,
	B_BIST,
	B_HEADTYPE,
	B_LAT,
	B_CACHESIZE
};


class PCI_Device {
protected:
	uint8_t m_bus, m_device, m_function;
	uint8_t m_baseclass, m_subclass;
public:
	PCI_Device();
	PCI_Device(uint8_t bus, uint8_t device, uint8_t function);
	uint16_t readConfigField16(PCI_FIELD16) const;
	uint16_t readConfigField8(PCI_FIELD8) const;
	uint32_t getConfigRegister(uint8_t offset) const;
	uint16_t getDeviceID() const;
	uint16_t getVendor() const;
	uint16_t getBaseClass() const;
	uint16_t getSubclass() const;
	bool isValid() const;
};

namespace PCI {
    gen::vector<PCI_Device> getDevices();
	void discover_devices();
};
