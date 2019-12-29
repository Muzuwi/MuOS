#include <Kernel/Debug/kdebugf.hpp>
#include <Kernel/Filesystem/VDM.hpp>
#include <LibGeneric/Vector.hpp>

//  TODO:  Use something other than vector?
static gen::vector<VirtualBlockDevice*> virtual_drives;

void VDM::register_drive(VirtualBlockDevice* drive) {
	if(!drive || virtual_drives.contains(drive)) return;

	kdebugf("[VDM] Registering drive at %x\n", (uint32_t)drive);
	virtual_drives.push_back(drive);

}

void VDM::unregister_drive(VirtualBlockDevice* drive) {
	assert(false);

	if(!virtual_drives.contains(drive)) {
		//  TODO: gen::vector does not provide removal of objects currently
		//  fix this when it does
	}
}

void VDM::debug() {
	kdebugf("[VDM] Saved devices: \n");
	for(size_t i = 0; i < virtual_drives.size(); i++) {
		kdebugf("	- %x\n", (uint32_t)virtual_drives[i]);
	}
}
