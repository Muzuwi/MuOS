#pragma once
#include <SystemTypes.hpp>

namespace core::fs::ext2 {
	struct BlockGroupDescriptor {
		uint32 block_bitmap;
		uint32 inode_bitmap;
		uint32 inode_table_start;
		uint16 unallocated_blocks;
		uint16 unallocated_inodes;
		uint16 directory_count;
		uint8 _unused[14];
	} __attribute__((packed));

	static_assert(sizeof(BlockGroupDescriptor) == 32, "block group descriptor should be 32 bytes");
}
