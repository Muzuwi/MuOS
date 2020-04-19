#include <Kernel/Device/PCI.hpp>
#include <Arch/i386/PortIO.hpp>
#include <Kernel/Debug/kdebugf.hpp>
#include <LibGeneric/Vector.hpp>

//  Devices found during detection
gen::vector<PCI_Device> pci_devices;

const char* pci_get_class_string(uint8_t base) {
	switch(base) {
		case 0x0: return "Unclassified";
		case 0x1: return "Mass Storage Controller";
		case 0x02: return "Network controller";
		case 0x03: return "Display controller";
		case 0x04: return "Multimedia controller";
		case 0x05: return "Memory controller";
		case 0x06: return "Bridge";
		case 0x07: return "Communication controller";
		case 0x08: return "Generic system peripheral";
		case 0x09: return "Input device controller";
		case 0x0a: return "Docking station";
		case 0x0b: return "Processor";
		case 0x0c: return "Serial bus controller";
		case 0x0d: return "Wireless controller";
		case 0x0e: return "Intelligent controller";
		case 0x0f: return "Satellite communications controller";
		case 0x10: return "Encryption controller";
		case 0x11: return "Signal processing controller";
		case 0x12: return "Processing accelerators";
		case 0x13: return "Non-Essential Instrumentation";
		case 0x40: return "Coprocessor";
		case 0xff: return "Unassigned";
		default:
			return "<unknown>";
	}
}

const char* pci_get_subclass_string(uint8_t base, uint8_t sub) {
	switch(base) {
		case 0x1: {
			switch(sub) {
				case 0x00: return "SCSI storage controller";
				case 0x01: return "IDE interface";
				case 0x02: return "Floppy disk controller";
				case 0x03: return "IPI bus controller";
				case 0x04: return "RAID bus controller";
				case 0x05: return "ATA controller";
				case 0x06: return "SATA controller";
				case 0x07: return "Serial Attached SCSI controller";
				case 0x08: return "Non-Volatile memory controller";
				case 0x80: return "Mass storage controller";
				default: return "<unknown>";
			}
		}
		case 0x2: {
			switch(sub) {
				case 0x00: return "Ethernet controller";
				case 0x01: return "Token ring network controller";
				case 0x02: return "FDDI network controller";
				case 0x03: return "ATM network controller";
				case 0x04: return "ISDN controller";
				case 0x05: return "WorldFip controller";
				case 0x06: return "PICMG controller";
				case 0x07: return "Infiniband controller";
				case 0x08: return "Fabric controller";
				case 0x80: return "Network controller";
				default: return "<unknown>";
			}
		}
		case 0x3: {
			switch(sub) {
				case 0x00: return "VGA compatible controller";
				case 0x01: return "XGA compatible controller";
				case 0x02: return "3D controller";
				case 0x80: return "Display controller";
				default: return "<unknown>";
			}
		}

		case 0x4: {
			switch(sub) {
				case 0x00: return "Multimedia video controller";
				case 0x01: return "Multimedia audio controller";
				case 0x02: return "Computer telephony device";
				case 0x03: return "Audio device";
				case 0x80: return "Multimedia controller";
				default: return "<unknown>";
			}
		}
		case 0x05: {
			switch(sub) {
				case 0x00: return "RAM Controller";
				case 0x01: return "Flash Controller";
				case 0x80: return "Other";

				default: return "<unknown>";
			}
		}
		case 0x06: {
			switch(sub) {
				case 0x00: return "Host Bridge";
				case 0x01: return "ISA Bridge";
				case 0x02: return "EISA Bridge";
				case 0x03: return "MCA Bridge";
				case 0x04: return "PCI-to-PCI Bridge";
				case 0x05: return "PCMCIA Bridge";
				case 0x06: return "NuBus Bridge";
				case 0x07: return "CardBus Bridge";
				case 0x08: return "RACEway Bridge";
				case 0x09: return "PCI-to-PCI Bridge";
				case 0x0A: return "InfiniBand-to-PCI Host Bridge";
				case 0x80: return "Other";

				default: return "<unknown>";
			}
		}
		case 0x07: {
			switch(sub) {
				case 0x00: return "Serial Controller";
				case 0x01: return "Parallel Controller";
				case 0x02: return "Multiport Serial Controller";
				case 0x03: return "Modem";
				case 0x04: return "IEEE 488.1/2 (GPIB) Controller";
				case 0x05: return "Smart Card";
				case 0x80: return "Other";

				default: return "<unknown>";
			}
		}
		case 0x08: {
			switch(sub) {
				case 0x00: return "PIC";
				case 0x01: return "DMA Controller";
				case 0x02: return "Timer";
				case 0x03: return "RTC Controller";
				case 0x04: return "PCI Hot-Plug Controller";
				case 0x05: return "SD Host controller";
				case 0x06: return "IOMMU";
				case 0x80: return "Other";

				default: return "<unknown>";
			}
		}
		case 0x09: {
			switch(sub) {
				case 0x00: return "Keyboard Controller";
				case 0x01: return "Digitizer Pen";
				case 0x02: return "Mouse Controller";
				case 0x03: return "Scanner Controller";
				case 0x04: return "Gameport Controller";
				case 0x80: return "Other";

				default: return "<unknown>";
			}
		}
		case 0x0A: {
			switch(sub) {
				case 0x00: return "Generic";
				case 0x80: return "Other";

				default: return "<unknown>";
			}
		}
		case 0x0B: {
			switch(sub) {
				case 0x00: return "386";
				case 0x01: return "486";
				case 0x02: return "Pentium";
				case 0x03: return "Pentium Pro";
				case 0x10: return "Alpha";
				case 0x20: return "PowerPC";
				case 0x30: return "MIPS";
				case 0x40: return "Co-Processor";
				case 0x80: return "Other";

				default: return "<unknown>";
			}
		}
		case 0x0C: {
			switch(sub) {
				case 0x00: return "FireWire (IEEE 1394) Controller";
				case 0x01: return "ACCESS Bus";
				case 0x02: return "SSA";
				case 0x03: return "USB Controller";
				case 0x04: return "Fibre Channel";
				case 0x05: return "SMBus";
				case 0x06: return "InfiniBand";
				case 0x07: return "IPMI Interface";
				case 0x08: return "SERCOS Interface (IEC 61491)";
				case 0x09: return "CANbus";
				case 0x80: return "Other";

				default: return "<unknown>";
			}
		}
		case 0x0D: {
			switch(sub) {
				case 0x00: return "iRDA Compatible Controller";
				case 0x01: return "Consumer IR Controller";
				case 0x10: return "RF Controller";
				case 0x11: return "Bluetooth Controller";
				case 0x12: return "Broadband Controller";
				case 0x20: return "Ethernet Controller (802.1a)";
				case 0x21: return "Ethernet Controller (802.1b)";
				case 0x80: return "Other";

				default: return "<unknown>";
			}
		}
		case 0x0E: {
			switch(sub) {
				case 0x00: return "I20";

				default: return "<unknown>";
			}
		}
		case 0x0F: {
			switch(sub) {
				case 0x01: return "Satellite TV Controller";
				case 0x02: return "Satellite Audio Controller";
				case 0x03: return "Satellite Voice Controller";
				case 0x04: return "Satellite Data Controller";

				default: return "<unknown>";
			}
		}
		case 0x10: {
			switch(sub) {
				case 0x00: return "Network and Computing Encrpytion/Decryption";
				case 0x10: return "Entertainment Encryption/Decryption";
				case 0x80: return "Other Encryption/Decryption";

				default: return "<unknown>";
			}
		}
		case 0x11: {
			switch(sub) {
				case 0x00: return "DPIO Modules";
				case 0x01: return "Performance Counters";
				case 0x10: return "Communication Synchronizer";
				case 0x20: return "Signal Processing Management";
				case 0x80: return "Other ";

				default: return "<unknown>";
			}
		}
		default:
			return "<unknown>";
	}
}

PCI_Device::PCI_Device() {

}

PCI_Device::PCI_Device(uint8_t bus, uint8_t device, uint8_t function) {
	m_bus = bus;
	m_device = device;
	m_function = function;

	m_baseclass = this->readConfigField8(B_CLASS);
	m_subclass = this->readConfigField8(B_SUBCLASS);
}


/*
 *	Reads a byte field from a PCI device register specified by the enum 'field'
 */
uint16_t PCI_Device::readConfigField8(PCI_FIELD8 field) const {
	switch(field) {
		case B_CLASS:
			return (this->getConfigRegister(0x08) & 0xFF000000) >> 24;
		case B_SUBCLASS:
			return (this->getConfigRegister(0x08) & 0x00FF0000) >> 16;
		case B_HEADTYPE:
			return (this->getConfigRegister(0x0C) & 0x00FF0000) >> 16;
		case B_REVISION:
			return (this->getConfigRegister(0x10) & 0x000000FF);
		case B_IF:
			return (this->getConfigRegister(0x08) & 0x0000ff00) >> 8;
		case B_BIST:
			return (this->getConfigRegister(0x0c) & 0xff000000) >> 24;
		case B_LAT:
			return (this->getConfigRegister(0x0c) & 0x0000ff00) >> 8;
		case B_CACHESIZE:
			return (this->getConfigRegister(0x0c) & 0x000000ff);
	}
	assert(false);
}


/*
 *	Reads a word field from a PCI device register specified by the enum 'field'
 */
uint16_t PCI_Device::readConfigField16(PCI_FIELD16 field) const {
	switch (field) {
		case W_DEVICEID:
			return (this->getConfigRegister(0x0) & 0xFFFF0000) >> 16;
		case W_VENDORID:
			return (this->getConfigRegister(0x0) & 0x0000FFFF);
		case W_STATUS:
			return (this->getConfigRegister(0x4) & 0xFFFF0000) >> 16;
		case W_COMMAND:
			return (this->getConfigRegister(0x4) & 0x0000FFFF);
	}
	assert(false);
}

/*
 *	Reads a configuration register at the specified offset
 */
uint32_t PCI_Device::getConfigRegister(uint8_t offset) const {
	uint32_t address = (uint32_t)((((uint32_t)m_bus << 16) |
	                               ((uint32_t)m_device << 11) |
	                               ((uint32_t)m_function << 8) |
	                               ((uint32_t)offset & (uint32_t)~0x3)) |
	                               ((uint32_t)0x80000000));
	outd(CONFIG_ADDRESS, address);
	return ind(CONFIG_DATA);
}


uint16_t PCI_Device::getVendor() const {
	return this->readConfigField16(W_VENDORID);
}

uint16_t PCI_Device::getBaseClass() const {
	return m_baseclass;
}

uint16_t PCI_Device::getSubclass() const {
	return m_subclass;
}

/*
 *	Returns true when the device represented by the object is valid
 */
bool PCI_Device::isValid() const{
	return (this->getVendor() != 0xFFFF);
}

uint16_t PCI_Device::getDeviceID() const {
	return this->readConfigField16(W_DEVICEID);
}


void check_device(uint8_t bus, uint8_t dev) {

	PCI_Device device(bus, dev, 0);

	if(!device.isValid()) return;

	pci_devices.push_back(device);

	//  Probe other functions if the device has them
	if(device.readConfigField8(B_HEADTYPE) & 0x80) {
		for(size_t function = 1; function < 8; function++) {
			device = PCI_Device(bus, dev, (uint8_t)function);
			if(device.isValid()) {
				//  TODO: Instead of treating them as a different device,
				//  would it be better to keep track of other functions
				//  in the main device class?
				pci_devices.push_back(device);
			}
		}
	}

}

/*
 *	Probe all the PCI buses and search for all the devices and their functions
 */
void PCI::discover_devices() {
	for(size_t bus = 0; bus < 256; bus++) {
		for(size_t dev = 0; dev < 32; dev++) {
			check_device((uint8_t)bus, (uint8_t)dev);
		}
	}
}

/*
 *	Returns a vector of all PCI devices found during discovery
 */
gen::vector<PCI_Device> PCI::getDevices() {
	return pci_devices;
}
