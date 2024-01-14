#pragma once
#include <LibGeneric/Vector.hpp>
#include "Core/Error/Error.hpp"
#include "Inode.hpp"
#include "Memory/KHeap.hpp"
#include "VFS.hpp"

class RamfsInode : public core::vfs::Inode {};

class RamfsFile : public core::vfs::File {
public:
	RamfsFile() = default;
};

class RamfsDirectory : public core::vfs::Directory {
public:
	RamfsDirectory() = default;

	core::Result<KRefPtr<Inode>> lookup(gen::String name) override {
		if(name == "foo" || name == "bar") {
			return KHeap::make<RamfsDirectory>();
		} else if(name == "baz") {
			return KHeap::make<RamfsFile>();
		}
		return core::Error::IOFail;
	}
};

class Ramfs : public core::vfs::FileSystem {
public:
	Ramfs()
	    : core::vfs::FileSystem(gen::String { "ramfs" }) {}

	core::Result<KRefPtr<core::vfs::DirectoryEntry>> mount() override {
		auto dentry = core::vfs::dentry_alloc();

		dentry->name = "ramfs";
		dentry->type = core::vfs::DirectoryEntry::Type::Positive;
		dentry->inode = KHeap::make<RamfsDirectory>();
		return { dentry };
	}
};
