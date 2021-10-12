#pragma once
#include <LibGeneric/Buffer.hpp>
#include <Kernel/SystemTypes.hpp>

enum class IOResult {
	OK,
	BufferTooSmall,
	InvalidAddress,
	DeviceFault
};

/*
 *	Abstract class representing a block device
 */
class VirtualBlockDevice {
public:
	/*
	 *	Reads a chunk of memory of the specified size from the drive at the specified address to the provided buffer
	 */
	virtual IOResult read(uint32_t address, size_t count, Buffer& read_buffer, size_t offset) = 0;

	/*
	 *	Writes a chunk of memory provided in an input buffer to the drive
	 */
	virtual IOResult write(uint32_t address, size_t count, Buffer& to_write, size_t offset) = 0;
};
