#pragma once
#include <Kernel/SystemTypes.hpp>

/*
 *	Abstract class representing a block device
 */
class VirtualBlockDevice {
public:
	/*
	 *	Reads a block from the drive at the specified address to the provided buffer
	 */
	virtual IOResult read_block(lba_t address, uintptr_t *destination_buffer, size_t size) = 0;

	/*
	 *	Writes a block provided in an input buffer to the drive
	 */
	virtual IOResult write_block(lba_t address, uintptr_t *source_buffer, size_t size) = 0;

	/*
	 *	Returns the block size used by the drive
	 */
	virtual size_t getBlockSize() = 0;
};
