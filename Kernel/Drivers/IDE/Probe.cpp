#include <Arch/x86_64/PCI/PCI.hpp>
#include <Arch/x86_64/PCI/PciDevice.hpp>
#include <Core/Error/Error.hpp>
#include <Core/IO/BlockDevice.hpp>
#include <Core/Object/Tree.hpp>
#include <Debug/klogf.hpp>
#include <Drivers/IDE/IDE.hpp>
#include <LibFormat/Format.hpp>
#include <LibGeneric/LockGuard.hpp>
#include <LibGeneric/Memory.hpp>
#include <Memory/KHeap.hpp>
#include <stddef.h>
#include <Structs/KFunction.hpp>
#include <SystemTypes.hpp>
#include "LibGeneric/String.hpp"

using namespace driver::ide;

static gen::String format_name(uint16 ctl_port, uint16 dev_port) {
	char output[16];
	Format::format("ide@{x}:{x}", output, sizeof(output), ctl_port, dev_port);
	return gen::String { output };
}

/**
 * 	Probe the specified I/O ports for an ATA PIO drive.
 */
static void ide_probe(uint16 disk_control, uint16 device_control) {
	//  FIXME/LEAK: Currently we're leaking both of the objects as there's no
	//  way to clean them up.
	auto* drive = KHeap::make<IdeDevice>(disk_control, device_control);
	if(!drive) {
		kerrorf("[driver::ide] IdeDevice allocation failed\n");
		return;
	}

	if(!drive->initialize()) {
		kerrorf("[driver::ide] Failed initializing drive @ IO {x}, {x}\n", disk_control, device_control);
		return;
	}

	auto* blk = KHeap::make<core::io::BlockDevice>(format_name(disk_control, device_control));
	if(!blk) {
		kerrorf("[driver::ide] Allocating BlockDevice failed\n");
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
		kerrorf("[driver::ide] BlockDevice object attach failed, core::Error: {x}\n", static_cast<size_t>(err));
		return;
	}
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
		klogf("[driver::ide] detect: No PCI IDE controllers detected\n");
		return core::Error::Ok;
	}

	auto dev = maybe_dev.unwrap();
	auto prog_if = dev.prog_if();
	kerrorf("[driver::ide] PCI IDE controller with device_id={x}, vendor_id={x}\n", dev.device_id(), dev.vendor_id());
	if(prog_if & 0b0101) {
		kerrorf("[driver::ide] PCI native mode unsupported, controller incompatible\n");
		return core::Error::Ok;
	}

	uint16 primary_base = 0x1F0, primary_base_control = 0x3F6;
	uint16 secondary_base = 0x170, secondary_base_control = 0x376;
	ide_probe(primary_base, primary_base_control);
	ide_probe(secondary_base, secondary_base_control);

	return core::Error::Ok;
}
