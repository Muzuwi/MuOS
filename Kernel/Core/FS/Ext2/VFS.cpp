#include "Core/FS/Ext2/VFS.hpp"
#include "Core/Error/Error.hpp"
#include "Core/FS/Ext2/DirectoryEntry.hpp"
#include "Core/FS/Ext2/Inode.hpp"
#include "Core/Log/Logger.hpp"
#include "Core/VFS/Inode.hpp"
#include "Core/VFS/VFS.hpp"
#include "Ext2FS.hpp"
#include "LibGeneric/ScopeGuard.hpp"
#include "LibGeneric/String.hpp"
#include "Memory/KHeap.hpp"

using namespace core::fs::ext2;

CREATE_LOGGER("fs::ext2::vfs", core::log::LogLevel::Debug);

core::Result<KRefPtr<VfsDirectory>> Ext2Fs::make_vfs_directory_for_inode(ninode_t inode) {
	//  FIXME: No sanity checks if inode is actually a directory
	auto vfsdir = KHeap::make<VfsDirectory>(this, inode);
	if(!vfsdir) {
		return core::Error::NoMem;
	}
	return vfsdir;
}

core::Result<KRefPtr<core::vfs::Inode>> Ext2Fs::directory_op_lookup(ninode_t ninode, gen::String name) {
	Inode inode;
	const auto err = read_inode(ninode, inode);
	if(err != Error::Ok) {
		::log.error("Reading inode failed, error: {}", static_cast<uintptr_t>(err));
		return core::Error::IOFail;
	}

	auto* buf = static_cast<uint8*>(KHeap::instance().chunk_alloc(inode.get_size()));
	DEFER {
		KHeap::instance().chunk_free(buf);
	};
	if(!buf) {
		return core::Error::NoMem;
	}
	if(const auto err = read_inode_data(ninode, buf, inode.get_size()); err != Error::Ok) {
		::log.error("Reading directory inode data failed, error: {}", static_cast<size_t>(err));
		return Error::IOFail;
	}

	auto* ptr = buf;
	while(ptr < (buf + inode.get_size())) {
		auto* dentry = reinterpret_cast<core::fs::ext2::DirectoryEntry*>(ptr);
		::log.debug("Child ./{} @ inode {x}", dentry->name, dentry->inode);
		//  We found our boy
		if(dentry->name == name) {
			auto result = make_vfs_directory_for_inode(dentry->inode);
			if(!result) {
				return result.error();
			}
			return result.destructively_move_data();
		}
		ptr += dentry->entry_size;
	}

	::log.debug("No child with required name: {} found", name.data());

	//  FIXME: Error: no such file or directory
	return nullptr;
}

core::Result<KRefPtr<core::vfs::DirectoryEntry>> Ext2Fs::mount() {
	auto dentry = core::vfs::dentry_alloc();
	if(!dentry) {
		return core::Error::NoMem;
	}

	dentry->name = "/";
	dentry->type = vfs::DirectoryEntry::Type::Positive;
	auto result = make_vfs_directory_for_inode(2);
	if(!result) {
		core::vfs::dentry_free(dentry);
		return result.error();
	}
	dentry->inode = result.destructively_move_data();

	return dentry;
}

core::Result<KRefPtr<core::vfs::Inode>> VfsDirectory::lookup(gen::String name) {
	return this->m_fs->directory_op_lookup(this->m_inode, name);
}
