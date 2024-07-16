#include <Arch/x86_64/PCI/PCI.hpp>
#include <Arch/x86_64/PCI/PciDevice.hpp>
#include <Core/IRQ/InterruptDisabler.hpp>
#include <Core/Log/Logger.hpp>
#include <LibGeneric/LockGuard.hpp>
#include <SystemTypes.hpp>

CREATE_LOGGER("x86_64::pci", core::log::LogLevel::Debug);

gen::StaticVector<PCI::PciDeviceLock, 512> PCI::s_devices {};
gen::Spinlock PCI::s_devices_lock {};

/*
 *  Initializes the PCI subsystem.
 */
void PCI::discover() {
	for(size_t bus = 0; bus < 256; bus++) {
		for(size_t dev = 0; dev < 32; dev++) {
			probe_device((uint8_t)bus, (uint8_t)dev);
		}
	}
}

void PCI::probe_device(uint8 bus, uint8 device) {
	auto dev = PciDevice { bus, device, 0 };
	if(dev.vendor_id() == 0xFFFF) {
		return;
	}
	register_device(dev);

	//  Device is not multi-function
	if(!(dev.header_type() & 0x80)) {
		return;
	}

	//  Probe other functions
	for(unsigned i = 1; i < 8; ++i) {
		auto child_dev = PciDevice { bus, device, static_cast<uint8>(i) };
		if(child_dev.vendor_id() == 0xFFFF) {
			continue;
		}
		register_device(child_dev);
	}
}

void PCI::register_device(PciDevice device) {
	log.info("{x}:{x}:{x} - Device={x}, Vendor={x}, Class={x}, Subclass={x}", device.bus(), device.device(),
	         device.function(), device.device_id(), device.vendor_id(), device.class_(), device.subclass());
	log.info("|- {} - {}", PCI::class_string(device.class_()),
	         PCI::subclass_string(device.class_(), device.subclass()));
	const auto header_type = device.header_type();
	const auto type = header_type & 0x7Fu;

	if(type == 0) {
		log.info("|- BAR0={x}, BAR1={x}, BAR2={x}", device.read_config(0x10), device.read_config(0x14),
		         device.read_config(0x18));
		log.info("|- BAR3={x}, BAR4={x}, BAR5={x}", device.read_config(0x1c), device.read_config(0x20),
		         device.read_config(0x24));
	} else if(type == 1) {
		log.info("|- BAR0={x}, BAR1={x}", device.read_config(0x10), device.read_config(0x14));
	} else {
		log.info("|- Unrecognized header type - {}", device.header_type());
	}

	if(header_type & 0x80u) {
		log.info("|- Multi-function device");
	}

	add_device(device);
}

void PCI::add_device(PciDevice device) {
	auto lock = PciDeviceLock { .device = device, .locked = false };
	s_devices.push_back(lock);
}

/*
 *  Try acquiring exclusive access to a specified PCI device.
 *  The callback should return true if a specific device is wanted.
 */
KOptional<PciDevice> PCI::acquire_device(gen::Function<bool(PciDevice const&)> callback) {
	core::irq::InterruptDisabler disabler {};
	gen::LockGuard guard { s_devices_lock };

	for(auto& lock : s_devices) {
		//  Ignore devices that were already acquired by other modules
		if(lock.locked) {
			continue;
		}

		if(callback(lock.device)) {
			lock.locked = true;
			return { lock.device };
		}
	}

	return { gen::nullopt };
}
