#pragma once
#include <Kernel/Filesystem/VirtualBlockDevice.hpp>

/*
 *	VDM - Virtual Disk Manager
 *	This handles registering of virtual drives provided by the specific device drivers
 *	and delegates them to the VFS (eventually).
 */
namespace VDM {
    /*
	 *	Registers a new VirtualBlockDevice
	 */
    void register_drive(VirtualBlockDevice* drive);

	/*
	 *	Unregisters a VirtualBlockDevice previously added to the VDM
	 *	This will be called when devices get removed from the system
	 */
	void unregister_drive(VirtualBlockDevice* drive);

	void debug();
}
