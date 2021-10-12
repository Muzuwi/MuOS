#pragma once
#include <Kernel/SystemTypes.hpp>

/*
 *	Abstract class for representing a file system on a drive
 */
class GenericFS {
public:
	virtual FSResult read(path_t path, void* buffer, size_t size, size_t offset);
	virtual FSResult write(path_t path, void* buffer, size_t size, size_t offset);
};
