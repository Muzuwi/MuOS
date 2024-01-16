#pragma once
#include "Core/VFS/Inode.hpp"

namespace core::fs::ext2 {
	class Ext2Fs;

	class VfsInode : public core::vfs::Inode {};

	class VfsFile : public core::vfs::File {
	public:
		VfsFile(Ext2Fs* fs, size_t inode)
		    : m_fs(fs)
		    , m_inode(inode) {}
	private:
		Ext2Fs* m_fs;
		size_t m_inode;
	};

	class VfsDirectory : public core::vfs::Directory {
	public:
		VfsDirectory(Ext2Fs* fs, size_t inode)
		    : m_fs(fs)
		    , m_inode(inode) {}

		core::Result<KRefPtr<core::vfs::Inode>> lookup(gen::String name) override;
	private:
		Ext2Fs* m_fs;
		size_t m_inode;
	};
}
