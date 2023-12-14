#pragma once
#include <SystemTypes.hpp>

namespace core::fs::ext2 {
	struct BaseSuperblock {
		uint32 inode_count;
		uint32 block_count;
		uint32 reserved_blocks;
		uint32 unallocated_blocks;
		uint32 unallocated_inodes;
		uint32 superblock_block_number;
		uint32 block_size_log;
		uint32 fragment_size_log;
		uint32 blocks_per_group;
		uint32 fragments_per_group;
		uint32 inodes_per_group;
		uint32 last_mount_time;
		uint32 last_write_time;
		uint16 mounts_since_check;
		uint16 mounts_before_check;
		uint16 ext2_signature;
		uint16 fs_state;
		uint16 action_on_error;
		uint16 ver_minor;
		uint32 time_of_last_check;
		uint32 interval_between_checks;
		uint32 creator_system;
		uint32 ver_major;
		uint16 reserved_UID;
		uint16 reserved_GID;

		constexpr uint64 version() const { return ((uint64_t)ver_major << 16u) | ver_minor; }
	} __attribute__((packed));

	struct ExtendedSuperblock {
		uint32 first_available_inode;
		uint16 inode_size;
		uint16 group;
		uint32 optional_features;
		uint32 required_features_to_RW;
		uint32 required_features_or_RO;
		uint8 fs_ID[16];
		char volume_name[16];
		char last_mounted[64];
		uint32 compression;
		uint8 prealloc_blocks_for_files;
		uint8 prealloc_blocks_for_dirs;
		uint16 nused1;
		uint8 journal_ID[16];
		uint32 journal_inode;
		uint32 journal_device;
		uint32 orphan_inode_head;
	} __attribute__((packed));

	struct Superblock {
		BaseSuperblock base;
		ExtendedSuperblock ext;
		uint8 _unused2[788];
	} __attribute__((packed));

	static_assert(sizeof(Superblock) == 1024, "the superblock should be 1024 bytes");
	static_assert(sizeof(BaseSuperblock) == 84, "base superblock should be 84 bytes");
	static_assert(sizeof(ExtendedSuperblock) == 152, "extended superblock should be 152 bytes");
}
