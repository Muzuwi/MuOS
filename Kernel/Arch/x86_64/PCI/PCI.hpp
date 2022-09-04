#pragma once
#include <Arch/x86_64/PCI/PciDevice.hpp>
#include <LibGeneric/Function.hpp>
#include <LibGeneric/List.hpp>
#include <LibGeneric/Spinlock.hpp>
#include <LibGeneric/StaticVector.hpp>
#include <Structs/KOptional.hpp>
#include <SystemTypes.hpp>

class PCI {
	struct PciDeviceLock {
		PciDevice device {};
		bool locked { false };
	};
	static gen::StaticVector<PciDeviceLock, 512> s_devices;
	static gen::Spinlock s_devices_lock;

	static void probe_device(uint8 bus, uint8 device);
	static void register_device(PciDevice device);
	static void add_device(PciDevice device);
public:
	static void discover();
	static KOptional<PciDevice> acquire_device(gen::Function<bool(PciDevice const&)>);
	static char const* class_string(uint16 class_);
	static char const* subclass_string(uint16 class_, uint16 subclass);

	static constexpr const uint16 config_address_port = 0xCF8;
	static constexpr const uint16 config_data_port = 0xCFC;
};