#include <Kernel/Device/IDE.hpp>
#include <Kernel/Device/IDE/IDE_Channel.hpp>
#include <Kernel/Device/IDE/IDE_Drive.hpp>
#include <Kernel/Filesystem/VDM.hpp>

gen::vector<IDE_Channel*> ide_channels;

/*
 *	Performs checks and initializes the devices on the first IDE controller
 */
void IDE::check_devices() {
	auto devices = PCI::getDevices();
	for(size_t i = 0; i < devices.size(); i++) {
		auto device = devices[i];
		if(device.getBaseClass() == IDE_CONTROLLER_BASE_CLASS && device.getSubclass() == IDE_CONTROLLER_SUBCLASS) {
			kdebugf("[ide] Found an IDE controller device\n");
		} else continue;

		PCI_Device main_controller = device;

		auto primary   = new IDE_Channel(main_controller, IDE_Channel_Type::Primary);
		auto secondary = new IDE_Channel(main_controller, IDE_Channel_Type::Secondary);

		if(!primary->valid() && !secondary->valid()) {
			kdebugf("[ide] Both channels returning bogus BARs, skipping\n");
			continue;
		}

		if(!primary->valid())
			kdebugf("[ide] Primary channel returning bogus BARs\n");

		if(!secondary->valid())
			kdebugf("[ide] Secondary channel returning bogus BARs\n");

		//  Disable IRQs
		if(primary->valid())
			primary->write_register(ATA_REG::Control, 2);

		if(secondary->valid())
			secondary->write_register(ATA_REG::Control, 2);

		//  Detect ATA-ATAPI devices
		if(primary->valid()){
			kdebugf("[ide] Looking for devices on primary channel\n");
			auto devices_primary = primary->find_drives();
			kdebugf("[ide] Found %i devices on primary channel\n", devices_primary.size());

			//  We don't care about the channel
			if(devices_primary.size() == 0) {
				delete primary;
			} else if(primary->valid()) {
				auto drives = primary->getDrives();

				for(size_t i = 0; i < drives.size(); i++)
					VDM::register_drive(drives[i]);

				//  The drive contains a pointer to the channel,
				//  save it to avoid leaking memory
				ide_channels.push_back(primary);
			}
		}

		if(secondary->valid()) {
			kdebugf("[ide] Looking for devices on secondary channel\n");
			auto devices_secondary = secondary->find_drives();
			kdebugf("[ide] Found %i devices on secondary channel\n", devices_secondary.size());

			//  We don't care about the channel
			if(devices_secondary.size() == 0) {
				delete primary;
			} else if(secondary->valid()) {
				auto drives = secondary->getDrives();

				for(size_t i = 0; i < drives.size(); i++)
					VDM::register_drive(drives[i]);

				//  The drive contains a pointer to the channel,
				//  save it to avoid leaking memory
				ide_channels.push_back(secondary);
			}

		}
	}
}
