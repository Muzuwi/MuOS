#pragma once
#include <SystemTypes.hpp>

namespace core::fs::ext2 {
	struct DirectoryEntry {
		uint32 inode;
		uint16 entry_size;
		uint8 name_len_minor;
		uint8 type_or_name_length;
		const char name[];
	} __attribute__((packed));
}
