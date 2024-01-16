#pragma once
#include <stddef.h>
#include <SystemTypes.hpp>
#include "BlockGroupDescriptor.hpp"
#include "Core/Error/Error.hpp"
#include "Core/FS/Ext2/VFS.hpp"
#include "Core/IO/BlockDevice.hpp"
#include "Core/VFS/Inode.hpp"
#include "Inode.hpp"
#include "LibGeneric/String.hpp"
#include "LibGeneric/Vector.hpp"
#include "Structs/KOptional.hpp"
#include "Superblock.hpp"

namespace core::fs::ext2 {
	using nblock_t = size_t;
	using ninode_t = size_t;
	using ngroup_t = size_t;

	class Ext2Fs : core::vfs::FileSystem {
	public:
		//  FIXME: DO NOT USE! USE PROBE INSTEAD!
		constexpr Ext2Fs(core::io::BlockDevice* bdev)
		    : core::vfs::FileSystem(gen::String("ext2fs@") + gen::String(bdev->name()))
		    , m_bdev(bdev) {}

		static core::Error probe(core::io::BlockDevice*);

		constexpr size_t inode_block_group(ninode_t inode) const { return (inode - 1) / m_inodes_per_group; }

		constexpr size_t inode_table_index(ninode_t inode) const { return (inode - 1) % m_inodes_per_group; }

		constexpr size_t inode_block(ninode_t inode) const {
			return (inode_table_index(inode) * m_inode_size) / m_block_size;
		}

		core::Result<KRefPtr<core::vfs::DirectoryEntry>> mount() override;
		core::Result<KRefPtr<VfsDirectory>> make_vfs_directory_for_inode(ninode_t);

		core::Result<KRefPtr<core::vfs::Inode>> directory_op_lookup(size_t inode, gen::String name);
	private:
		core::io::BlockDevice* m_bdev { nullptr };
		Superblock* m_superblock { nullptr };
		gen::Vector<BlockGroupDescriptor*> m_block_group_descriptors {};

		size_t m_device_block_size;
		size_t m_block_size;
		size_t m_inode_size;
		size_t m_blocks_per_group;
		size_t m_inodes_per_group;
		size_t m_inodes_per_block;
		size_t m_block_group_count;
		size_t m_BGDT_block;
	private:
		core::Error read_superblock();
		core::Error read_block(nblock_t block, uint8* dest, size_t destlen, size_t n, size_t offset);
		core::Error read_inode(ninode_t inode, Inode& output);
		core::Error read_inode_data(ninode_t inode, uint8* dest, size_t destlen);
		core::Error read_contents_singly_indirect(nblock_t, uint8* dest, size_t destlen, size_t& offset,
		                                          size_t& bytes_left);
		core::Error read_contents_doubly_indirect(nblock_t, uint8* dest, size_t destlen, size_t& offset,
		                                          size_t& bytes_left);
		core::Error read_contents_triply_indirect(nblock_t, uint8* dest, size_t destlen, size_t& offset,
		                                          size_t& bytes_left);
	};
}
