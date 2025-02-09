#include <Arch/x86_64/PCI/PCI.hpp>
#include <Arch/x86_64/PCI/PciDevice.hpp>
#include <Core/Error/Error.hpp>
#include <Core/IO/BlockDevice.hpp>
#include <Core/Log/Logger.hpp>
#include <Core/Mem/Heap.hpp>
#include <Core/Object/Tree.hpp>
#include <Drivers/IDE/IDE.hpp>
#include <LibFormat/Format.hpp>
#include <LibGeneric/LockGuard.hpp>
#include <LibGeneric/Memory.hpp>
#include <stddef.h>
#include <Structs/KFunction.hpp>
#include <SystemTypes.hpp>
#include "LibGeneric/String.hpp"

using namespace driver::ide;
CREATE_LOGGER("x86_64::ide::probe", core::log::LogLevel::Debug);

static gen::String format_name(uint16 ctl_port, uint16 dev_port, DriveSelect which) {
	char output[16];
	Format::format("ide@{x}:{x}:{}", output, sizeof(output), ctl_port, dev_port,
	               which == driver::ide::DriveSelect::Master ? "master" : "slave");
	return gen::String { output };
}

/**
 * 	Probe the specified I/O ports for an ATA PIO drive.
 */
static void ide_probe(uint16 disk_control, uint16 device_control, driver::ide::DriveSelect which) {
	//  FIXME/LEAK: Currently we're leaking both of the objects as there's no
	//  way to clean them up.
	auto* drive = core::mem::make<IdeDevice>(disk_control, device_control, which);
	if(!drive) {
		log.error("probe: IdeDevice allocation failed\n");
		return;
	}

	if(!drive->initialize()) {
		log.warning("probe: Failed initializing drive @ {x}:{x}:{}", disk_control, device_control,
		            which == driver::ide::DriveSelect::Master ? 'm' : 's');
		return;
	}

	auto* blk = core::mem::make<core::io::BlockDevice>(format_name(disk_control, device_control, which));
	if(!blk) {
		log.error("probe: Allocating BlockDevice failed");
		return;
	}

	gen::construct_at(&blk->read,
	                  [drive](uint8* dest, size_t dest_len, core::io::block_t blk,
	                          core::io::block_count_t blkcount) -> core::Error {
		                  if(blkcount > 0xFFFF) {
			                  return core::Error::InvalidArgument;
		                  }
		                  return drive->access(blk, blkcount, dest, dest_len, Direction::Read);
	                  });
	gen::construct_at(&blk->write,
	                  [drive](uint8 const* src, size_t src_len, core::io::block_t blk,
	                          core::io::block_count_t blkcount) -> core::Error {
		                  if(blkcount > 0xFFFF) {
			                  return core::Error::InvalidArgument;
		                  }
		                  //  Evil const_cast, the buffer isn't modified however as we're doing a write
		                  auto* src_without_const = const_cast<uint8*>(src);
		                  return drive->access(blk, blkcount, src_without_const, src_len, Direction::Write);
	                  });
	gen::construct_at(&blk->blksize, [drive](size_t& output) -> core::Error {
		output = drive->sector_size();
		return core::Error::Ok;
	});
	gen::construct_at(&blk->blkcount, [drive](size_t& output) -> core::Error {
		output = drive->sectors();
		return core::Error::Ok;
	});

	if(const auto err = core::obj::attach(blk); err != core::Error::Ok) {
		log.error("probe: BlockDevice object attach failed, core::Error: {}", err);
		return;
	}
	log.info("probe: Successfully attached block device '{}'", blk->name());
}

/**
 * 	Initialize the IDE subsystem.
 * 	Perform initial probing of available ATA PIO drives and
 * 	register valid ones in the subsystem.
 */
core::Error driver::ide::init() {
	auto maybe_dev = PCI::acquire_device(
	        [](PciDevice const& device) -> bool { return device.class_() == 0x01 && device.subclass() == 0x01; });
	if(!maybe_dev.has_value()) {
		log.warning("init: No PCI IDE controllers detected");
		return core::Error::Ok;
	}

	auto dev = maybe_dev.unwrap();
	auto prog_if = dev.prog_if();
	log.info("init: PCI IDE controller with device_id={x}, vendor_id={x}", dev.device_id(), dev.vendor_id());

	if(!(prog_if & 0b0001)) {
		uint16 primary_base = 0x1F0, primary_base_control = 0x3F6;
		ide_probe(primary_base, primary_base_control, DriveSelect::Master);
		ide_probe(primary_base, primary_base_control, DriveSelect::Slave);
	}
	if(!(prog_if & 0b0100)) {
		uint16 secondary_base = 0x170, secondary_base_control = 0x376;
		ide_probe(secondary_base, secondary_base_control, DriveSelect::Master);
		ide_probe(secondary_base, secondary_base_control, DriveSelect::Slave);
	}
	return core::Error::Ok;
}
