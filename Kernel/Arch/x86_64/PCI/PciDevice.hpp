#pragma once
#include "SystemTypes.hpp"

class PciDevice {
	uint8 m_bus;
	uint8 m_device;
	uint8 m_function;
public:
	constexpr PciDevice() = default;

	constexpr PciDevice(uint8 bus, uint8 device, uint8 function)
	    : m_bus(bus)
	    , m_device(device)
	    , m_function(function) {}

	void write_config(size_t offset, uint32 value);
	uint32 read_config(size_t offset) const;

	uint16 device_id() const { return read_config(0x0) >> 16u; }
	uint16 vendor_id() const { return read_config(0x0) & 0xFFFFu; }
	uint8 class_() const { return read_config(0x8) >> 24u; }
	uint8 subclass() const { return (read_config(0x8) >> 16u) & 0xFFu; }
	uint8 header_type() const { return (read_config(0xC) >> 16u) & 0xFFu; }
	constexpr uint8 bus() const { return m_bus; }
	constexpr uint8 device() const { return m_device; }
	constexpr uint8 function() const { return m_function; }
};