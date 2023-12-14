#pragma once
#include <stddef.h>
#include <stdint.h>
#include <SystemTypes.hpp>

namespace core::fs::ext2 {
	enum InodeType : uint16 {
		FIFO = 0x1000,
		CharacterDevice = 0x2000,
		Directory = 0x4000,
		BlockDevice = 0x6000,
		Regular = 0x8000,
		SymbolicLink = 0xA000,
		Socket = 0xc000
	};

	enum InodePermissions : uint16 {
		Other_X = 0x001,
		Other_W = 0x002,
		Other_R = 0x004,

		Group_X = 0x008,
		Group_W = 0x010,
		Group_R = 0x020,

		User_X = 0x040,
		User_W = 0x080,
		User_R = 0x100,

		Sticky = 0x200,
		SetGID = 0x400,
		SetUID = 0x800
	};

	struct Inode {
		uint16_t type;
		uint16_t UID;
		uint32_t lower_size;
		uint32_t last_access;
		uint32_t created;
		uint32_t last_modified;
		uint32_t deletion_time;
		uint16_t GID;
		uint16_t hard_link_count;
		uint32_t sector_count;
		uint32_t flags;
		uint32_t os_specific_1;
		uint32_t direct_block_pointers[12];
		uint32_t singly_indirect_block_ptr;
		uint32_t doubly_indirect_block_ptr;
		uint32_t triply_indirect_block_ptr;
		uint32_t generation_number;
		uint32_t extended_attributes;
		uint32_t upper_size_or_acl;
		uint32_t fragment_block_addr;
		uint8_t os_specific_2[12];

		inline size_t get_size() const {
			if(type & InodeType::Directory) {
				return lower_size;
			} else {
				return ((size_t)upper_size_or_acl << 32u) | lower_size;
			}
		}

		inline constexpr bool is_directory() const { return type & InodeType::Directory; }

		inline constexpr bool is_regular_file() const { return type & InodeType::Regular; }

		inline constexpr bool is_char_device() const { return type & InodeType::CharacterDevice; }

		inline constexpr bool is_block_device() const { return type & InodeType::BlockDevice; }

		inline constexpr bool is_FIFO() const { return type & InodeType::FIFO; }

		inline constexpr bool is_socket() const { return type & InodeType::Socket; }

		inline constexpr bool is_symlink() const { return type & InodeType::SymbolicLink; }

	} __attribute__((packed));

	static_assert(sizeof(Inode) == 128, "the inode should be 128 bytes");
}
