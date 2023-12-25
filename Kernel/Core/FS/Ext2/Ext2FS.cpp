#include "Ext2FS.hpp"
#include <Core/Error/Error.hpp>
#include <Core/FS/Ext2/BlockGroupDescriptor.hpp>
#include <Core/FS/Ext2/DirectoryEntry.hpp>
#include <Core/FS/Ext2/Inode.hpp>
#include <Core/FS/Ext2/Superblock.hpp>
#include <Core/IO/BlockDevice.hpp>
#include <Core/Log/Logger.hpp>
#include <Debug/kassert.hpp>
#include <LibFormat/Formatters/Pointer.hpp>
#include <LibGeneric/Move.hpp>
#include <Memory/KHeap.hpp>
#include <stddef.h>
#include <stdint.h>
#include <Structs/KOptional.hpp>
#include <SystemTypes.hpp>
#include "LibGeneric/ScopeGuard.hpp"

using namespace core::fs::ext2;
CREATE_LOGGER("fs::ext2", core::log::LogLevel::Debug);

core::Error core::fs::ext2::Ext2Fs::probe(core::io::BlockDevice* bdev) {
	if(!bdev) {
		return core::Error::InvalidArgument;
	}

	auto* fs = KHeap::make<Ext2Fs>(bdev);
	if(!fs) {
		return core::Error::NoMem;
	}

	::log.debug("Reading superblock of device");
	if(const auto err = fs->read_superblock(); err != core::Error::Ok) {
		return core::Error::IOFail;
	}

	::log.debug("Ext2 version: {x}", fs->m_superblock->base.version());
	::log.debug("Blocks per group: {}, inodes per group: {}", fs->m_blocks_per_group, fs->m_inodes_per_group);
	::log.debug("Ext2 block size: {}, inode size: {}", fs->m_block_size, fs->m_inode_size);
	::log.debug("Ext2 block count: {}, inode count: {}", fs->m_superblock->base.block_count,
	            fs->m_superblock->base.inode_count);
	::log.debug("Block group count: {}", fs->m_block_group_count);

	for(auto& bgd : fs->m_block_group_descriptors) {
		::log.debug("Group start: {x}", bgd->inode_table_start);
	}

	Inode inode;
	const auto err = fs->read_inode(2, inode);
	if(err != Error::Ok) {
		::log.error("Reading root inode failed, error: {}", static_cast<uintptr_t>(err));
		return core::Error::IOFail;
	}

	auto type_str = [](uint16_t type) -> char const* {
		type &= 0xf000u;
		switch(type) {
			case FIFO: return "FIFO";
			case CharacterDevice: return "CharacterDevice";
			case Directory: return "Directory";
			case BlockDevice: return "BlockDevice";
			case Regular: return "Regular";
			case SymbolicLink: return "SymbolicLink";
			case Socket: return "Socket";
			default: return "NaN";
		}
	};

	::log.debug("RootInode: Type: {x}", type_str(inode.type));
	::log.debug("RootInode: UID={} GID={}", inode.UID, inode.GID);
	::log.debug("RootInode: Created: {x}", inode.created);
	::log.debug("RootInode: Size: {x}", inode.get_size());

	auto* buf = static_cast<uint8*>(KHeap::instance().chunk_alloc(inode.get_size()));
	if(!buf) {
		return core::Error::NoMem;
	}
	if(const auto err = fs->read_inode_data(2, buf, inode.get_size()); err != Error::Ok) {
		return Error::IOFail;
	}

	KOptional<ninode_t> where_sparse {};

	auto* ptr = buf;
	while(ptr < (buf + inode.get_size())) {
		auto* dentry = reinterpret_cast<DirectoryEntry*>(ptr);
		::log.debug("/{} @ inode {x}", dentry->name, dentry->inode);
		ptr += dentry->entry_size;
		if(gen::String { dentry->name } == "sparse.img") {
			where_sparse = dentry->inode;
		}
	}

	if(!where_sparse.has_value()) {
		return Error::IOFail;
	}

	if(const auto err = fs->read_inode(where_sparse.unwrap(), inode); err != Error::Ok) {
		return Error::IOFail;
	}
	::log.debug("Sparse - Type: {x}", type_str(inode.type));
	::log.debug("Sparse - UID={} GID={}", inode.UID, inode.GID);
	::log.debug("Sparse - Created: {x}", inode.created);
	::log.debug("Sparse - Size: {x}", inode.get_size());
	::log.debug("Sparse - Direct Block Pointers:");
	for(auto i = 0; i < 12; ++i) {
		::log.debug("{x} ", inode.direct_block_pointers[i]);
	}
	::log.debug("");

	::log.debug("Sparse - Singly Indirect Block Pointer {x}:", inode.singly_indirect_block_ptr);
	::log.debug("Sparse - Doubly Indirect Block Pointer {x}:", inode.doubly_indirect_block_ptr);
	::log.debug("Sparse - Triply Indirect Block Pointer {x}:", inode.triply_indirect_block_ptr);
	if(inode.singly_indirect_block_ptr != 0) {
		const size_t pointers_per_block = fs->m_block_size / sizeof(uint32);
		auto* block_pointers = KHeap::allocate(fs->m_block_size);
		if(!block_pointers) {
			return core::Error::NoMem;
		}

		const auto err = fs->read_block(inode.singly_indirect_block_ptr, static_cast<uint8*>(block_pointers),
		                                fs->m_block_size, fs->m_block_size, 0);
		if(err != Error::Ok) {
			KHeap::free(block_pointers);
			return core::Error::IOFail;
		}

		for(size_t i = 0; i < pointers_per_block; ++i) {
			const uint32 data_block = *(static_cast<uint32*>(block_pointers) + i);
			::log.debug("{x} ", data_block);
		}
		KHeap::free(block_pointers);
	}

	return core::Error::Ok;
}

core::Error Ext2Fs::read_superblock() {
	m_superblock = KHeap::make<Superblock>();
	if(!m_superblock) {
		return Error::NoMem;
	}

	if(const auto err = m_bdev->blksize(m_device_block_size); err != Error::Ok) {
		return err;
	}

	if((1024 % m_device_block_size) != 0) {
		//  For now assume devices with nice block sizes
		return Error::InvalidArgument;
	}

	auto which_block = 1024 / m_device_block_size;
	auto how_many = 1024 / m_device_block_size;
	if(const auto err = m_bdev->read(reinterpret_cast<uint8*>(m_superblock), sizeof(Superblock), gen::move(which_block),
	                                 gen::move(how_many));
	   err != Error::Ok) {
		return err;
	}

	if(m_superblock->base.ext2_signature != 0xef53) {
		::log.error("Ext2 signature invalid! Got {x}", m_superblock->base.ext2_signature);
		return Error::IOFail;
	}

	m_blocks_per_group = m_superblock->base.blocks_per_group;
	m_inodes_per_group = m_superblock->base.inodes_per_group;
	m_inode_size = (m_superblock->base.version() < 1) ? 128 : m_superblock->ext.inode_size;
	m_block_size = 1024u << m_superblock->base.block_size_log;
	m_inodes_per_block = m_block_size / m_inode_size;
	//  FIXME:  Use an actual rounding function instead of this
	//  FIXME: WHY IS THERE A DOUBLE IN THE KERNEL
	m_block_group_count = (size_t)((m_superblock->base.block_count / (double)m_blocks_per_group) + 0.5f);
	m_BGDT_block = m_block_size == 1024u ? 2 : 1;

	//  Preload all block group descriptors
	const size_t table_size = m_block_group_count * sizeof(BlockGroupDescriptor);
	const size_t table_size_in_blocks = (table_size + m_block_size - 1) / m_block_size;
	const size_t buf_size = table_size_in_blocks * m_block_size;
	auto* table = KHeap::instance().chunk_alloc(buf_size);
	if(!table) {
		return Error::NoMem;
	}
	DEFER {
		KHeap::instance().chunk_free(table);
	};
	for(auto idx = 0; idx < table_size_in_blocks; ++idx) {
		if(const auto err = read_block(m_BGDT_block + idx, reinterpret_cast<uint8*>(table), buf_size, m_block_size,
		                               m_block_size * idx);
		   err != Error::Ok) {
			return Error::IOFail;
		}
	}

	for(auto i = 0; i < m_block_group_count; ++i) {
		auto* bgd = KHeap::make<BlockGroupDescriptor>();
		if(!bgd) {
			return Error::NoMem;
		}
		memcpy(bgd, reinterpret_cast<BlockGroupDescriptor*>(table) + i, sizeof(BlockGroupDescriptor));
		m_block_group_descriptors.push_back(bgd);
	}

	return core::Error::Ok;
}

core::Error Ext2Fs::read_block(nblock_t block, uint8* dest, size_t destlen, size_t n, size_t offset) {
	::log.debug("read_block blk={x} dest={x} destlen={x} n={x} offset={x}", block, Format::ptr(dest), destlen, n,
	            offset);

	//  Sanity checks
	if(n > m_block_size) {
		::log.warning("BUG: Calling read_block with n > block size ({} > {})", n, m_block_size);
		return core::Error::InvalidArgument;
	}
	if(offset + n > destlen) {
		::log.warning("BUG: read_block operation of size {} would overflow buffer of length {x} with offset {x}", n,
		              destlen, offset);
		return core::Error::InvalidArgument;
	}

	//  Convert to device blocks
	auto which_device_block = (block * m_block_size) / m_device_block_size;
	size_t how_many_device_blocks = (m_block_size / m_device_block_size);
	if(how_many_device_blocks == 0) {
		how_many_device_blocks = 1;
	}

	//  TODO: Optimize allocations
	auto* device_buffer = KHeap::instance().chunk_alloc(m_block_size);
	if(!device_buffer) {
		return core::Error::NoMem;
	}
	DEFER {
		KHeap::instance().chunk_free(device_buffer);
	};

	auto* buf = static_cast<uint8*>(device_buffer);
	size_t buflen = m_block_size;

	const auto err = m_bdev->read(gen::move(buf), gen::move(buflen), gen::move(which_device_block),
	                              gen::move(how_many_device_blocks));
	if(err != Error::Ok) {
		::log.error("read_block failed: Reading from block device returned error ({})", static_cast<size_t>(err));
		return core::Error::IOFail;
	}

	memcpy(dest + offset, buf, n);

	return core::Error::Ok;
}

core::Error Ext2Fs::read_inode(ninode_t ninode, Inode& output) {
	const auto ngroup = inode_block_group(ninode);
	if(ngroup >= m_block_group_descriptors.size()) {
		::log.warning("BUG: When reading inode {x}: group {x} is not present!", ninode, ngroup);
		return core::Error::IOFail;
	}

	auto* descriptor = m_block_group_descriptors[ngroup];
	const auto inode_start = descriptor->inode_table_start;
	const auto block = inode_block(ninode);
	const auto inode_in_block = inode_start + block;
	const auto index_in_block = inode_table_index(ninode) % (m_inodes_per_block);
	::log.debug("Inode {} at group {}", ninode, ngroup);
	::log.debug("Block bitmap at {}", descriptor->block_bitmap);
	::log.debug("Inode bitmap at {}", descriptor->inode_bitmap);
	::log.debug("Free blocks: {}", descriptor->unallocated_blocks);
	::log.debug("Group inode table starts at {}", inode_in_block);
	::log.debug("Inode is in table at index {}", index_in_block);

	auto* buf = KHeap::allocate(m_block_size);
	if(!buf) {
		return core::Error::NoMem;
	}
	DEFER {
		KHeap::free(buf);
	};

	const auto err = read_block(inode_in_block, static_cast<uint8*>(buf), m_block_size, m_block_size, 0);
	if(err != Error::Ok) {
		return core::Error::IOFail;
	}

	auto* inode_ptr = static_cast<uint8*>(buf);
	inode_ptr += index_in_block * sizeof(Inode);
	memcpy(&output, inode_ptr, sizeof(Inode));

	return core::Error::Ok;
}

core::Error Ext2Fs::read_inode_data(ninode_t ninode, uint8* dest, size_t destlen) {
	auto inode = Inode {};
	if(const auto err = read_inode(ninode, inode); err != Error::Ok) {
		return err;
	}

	const size_t inode_size = inode.get_size();

	if(destlen < inode.get_size()) {
		return core::Error::InvalidArgument;
	}

	size_t bytes_left = inode_size;
	size_t offset = 0;

	for(size_t i = 0; i < 12; ++i) {
		const size_t block_pointer = inode.direct_block_pointers[i];
		if(block_pointer == 0) {
			continue;
		}

		const size_t copy_size = bytes_left < m_block_size ? bytes_left : m_block_size;
		if(const auto err = read_block(block_pointer, dest, destlen, copy_size, offset); err != Error::Ok) {
			//  FIXME: Better handle I/O errors / retry
			return core::Error::IOFail;
		}
		bytes_left -= copy_size;
		offset += copy_size;
	}

	if(inode.singly_indirect_block_ptr != 0) {
		const auto err =
		        read_contents_singly_indirect(inode.singly_indirect_block_ptr, dest, destlen, offset, bytes_left);
		if(err != Error::Ok) {
			//  FIXME: Better handle I/O errors / retry
			return core::Error::IOFail;
		}
	}

	if(inode.doubly_indirect_block_ptr != 0) {
		const auto err =
		        read_contents_doubly_indirect(inode.doubly_indirect_block_ptr, dest, destlen, offset, bytes_left);
		if(err != Error::Ok) {
			//  FIXME: Better handle I/O errors / retry
			return core::Error::IOFail;
		}
	}

	if(inode.triply_indirect_block_ptr != 0) {
		const auto err =
		        read_contents_triply_indirect(inode.triply_indirect_block_ptr, dest, destlen, offset, bytes_left);
		if(err != Error::Ok) {
			//  FIXME: Better handle I/O errors / retry
			return core::Error::IOFail;
		}
	}

	if(bytes_left != 0) {
		::log.error("FS CORRUPTED: Exhausted all indirect pointers, but {} bytes left for reading!", bytes_left);
		return core::Error::IOFail;
	}

	return core::Error::Ok;
}

core::Error Ext2Fs::read_contents_singly_indirect(nblock_t nblock, uint8* dest, size_t destlen, size_t& offset,
                                                  size_t& bytes_left) {
	const size_t pointers_per_block = m_block_size / sizeof(uint32);
	auto* block_pointers = KHeap::allocate(m_block_size);
	if(!block_pointers) {
		return core::Error::NoMem;
	}
	DEFER {
		KHeap::free(block_pointers);
	};

	const auto err = read_block(nblock, static_cast<uint8*>(block_pointers), m_block_size, m_block_size, 0);
	if(err != Error::Ok) {
		return core::Error::IOFail;
	}

	size_t written = 0;
	for(size_t i = 0; i < pointers_per_block; ++i) {
		const uint32 data_block = *(static_cast<uint32*>(block_pointers) + i);
		if(data_block == 0) {
			continue;
		}

		const size_t copy_size = bytes_left - written < m_block_size ? bytes_left - written : m_block_size;
		if(offset + copy_size > destlen) {
			::log.warning(
			        "BUG: Block {x} read operation at offset {x} with copy size {x} would overflow max buffer size of {x}",
			        nblock, offset, copy_size, destlen);
			return core::Error::InvalidArgument;
		}

		if(const auto err = read_block(data_block, dest, destlen, copy_size, offset); err != Error::Ok) {
			return core::Error::IOFail;
		}
		written += copy_size;
		offset += copy_size;
		bytes_left -= copy_size;
	}

	return core::Error::Ok;
}

core::Error Ext2Fs::read_contents_doubly_indirect(nblock_t nblock, uint8* dest, size_t destlen, size_t& offset,
                                                  size_t& bytes_left) {
	const size_t pointers_per_block = m_block_size / sizeof(uint32);
	auto* block_pointers = KHeap::allocate(m_block_size);
	if(!block_pointers) {
		return core::Error::NoMem;
	}
	DEFER {
		KHeap::free(block_pointers);
	};

	const auto err = read_block(nblock, static_cast<uint8*>(block_pointers), m_block_size, m_block_size, 0);
	if(err != Error::Ok) {
		return core::Error::IOFail;
	}

	for(size_t i = 0; i < pointers_per_block; ++i) {
		const uint32 data_block = *(static_cast<uint32*>(block_pointers) + i);
		if(data_block == 0) {
			continue;
		}

		if(const auto err = read_contents_singly_indirect(data_block, dest, destlen, offset, bytes_left);
		   err != core::Error::Ok) {
			::log.error(
			        "ERROR: During doubly indirect read: Reading single indirect block data pointer {x} returned an error ({})",
			        data_block, static_cast<uintptr_t>(err));
			return core::Error::IOFail;
		}
	}

	return core::Error::Ok;
}

core::Error Ext2Fs::read_contents_triply_indirect(nblock_t nblock, uint8* dest, size_t destlen, size_t& offset,
                                                  size_t& bytes_left) {
	const size_t pointers_per_block = m_block_size / sizeof(uint32);
	auto* block_pointers = KHeap::allocate(m_block_size);
	if(!block_pointers) {
		return core::Error::NoMem;
	}
	DEFER {
		KHeap::free(block_pointers);
	};

	const auto err = read_block(nblock, static_cast<uint8*>(block_pointers), m_block_size, m_block_size, 0);
	if(err != Error::Ok) {
		return core::Error::IOFail;
	}

	for(size_t i = 0; i < pointers_per_block; ++i) {
		const uint32 data_block = *(static_cast<uint32*>(block_pointers) + i);
		if(data_block == 0) {
			continue;
		}

		if(const auto err = read_contents_doubly_indirect(data_block, dest, destlen, offset, bytes_left);
		   err != core::Error::Ok) {
			::log.error(
			        "ERROR: During triply indirect read: Reading doubly indirect block data pointer {x} returned an error ({})",
			        data_block, static_cast<uintptr_t>(err));
			return core::Error::IOFail;
		}
	}

	return core::Error::Ok;
}
