#include <Kernel/Device/IDE.hpp>
#include <Kernel/Device/IDE/IDE_Channel.hpp>
#include <Kernel/Device/IDE/IDE_Drive.hpp>
#include <Kernel/Filesystem/VDM.hpp>

gen::vector<IDE_Channel*> ide_channels;

/*
 *	Performs checks and initializes the devices on the first IDE controller
 */
void IDE::check_devices() {
	auto devices = PCI::getDevices().filter([](const PCI_Device& dev) -> bool{
		return dev.getBaseClass() == IDE_CONTROLLER_BASE_CLASS &&
			   dev.getSubclass() == IDE_CONTROLLER_SUBCLASS;
	});

	if(devices.size() != 1) {
		kdebugf("[ide] no IDE controller present\n");
		return;
	}

	PCI_Device main_controller = devices[0];

	auto primary   = new IDE_Channel(main_controller, IDE_Channel_Type::Primary);
	auto secondary = new IDE_Channel(main_controller, IDE_Channel_Type::Secondary);

	if(!primary->valid() && !secondary->valid()) {
		kdebugf("[ide] Both channels returning bogus BARs, skipping\n");
		return;
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

		//  We don't care about the channel
		if(devices_secondary.size() == 0) {
			delete secondary;
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
