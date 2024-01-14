#include <Core/VFS/Inode.hpp>
#include <Core/VFS/VFS.hpp>
#include <Debug/kassert.hpp>
#include "Core/Error/Error.hpp"
#include "Core/Log/Logger.hpp"
#include "Core/VFS/Ramfs.hpp"
#include "Debug/kpanic.hpp"
#include "LibGeneric/Memory.hpp"
#include "LibGeneric/String.hpp"
#include "Memory/KHeap.hpp"
#include "Structs/KResult.hpp"

CREATE_LOGGER("vfs", core::log::LogLevel::Debug);

static KRefPtr<core::vfs::FileSystem> s_filesystem;
static KRefPtr<core::vfs::DirectoryEntry> s_root;

core::vfs::DirectoryEntry* core::vfs::dentry_alloc() {
	auto ptr = static_cast<core::vfs::DirectoryEntry*>(KHeap::instance().slab_alloc(sizeof(core::vfs::DirectoryEntry)));
	gen::construct_at(ptr);
	return ptr;
}

void core::vfs::dentry_free(core::vfs::DirectoryEntry* ptr) {
	KHeap::instance().slab_free(ptr, sizeof(core::vfs::DirectoryEntry));
}

static void mount_root() {
	if(!s_filesystem) {
		log.fatal("No root filesysten - panic!");
		kpanic();
	}

	auto result = s_filesystem->mount();
	if(!result) {
		log.fatal("Could not allocate root inode!");
		kpanic();
	}
	s_root = result.destructively_move_data();
}

//  FIXME: Horribly inefficient
//  This would be better with a stringview-like interface
static auto pop_component(gen::String path) {
	struct ReturnValue {
		gen::String component;
		gen::String rest;
	};

	gen::String current_component {};
	gen::String leftovers {};

	size_t idx = 0;
	//  Skip over any leading path separators
	for(; idx < path.size(); ++idx) {
		if(path[idx] != '/') {
			break;
		}
	}
	//	Keep adding component chars until a path separator
	for(; idx < path.size(); ++idx) {
		current_component += path[idx];
	}
	//  Leftover path values
	for(; idx < path.size(); ++idx) {
		leftovers += path[idx];
	}

	log.debug("Component: {} | Rest: {}", current_component.data(), leftovers.data());
	return ReturnValue { .component = current_component, .rest = leftovers };
}

/**	Follow a path, component by component, starting from the dentry
 * 	at `start`, creating dentries for each subcomponent when required.
 */
static core::Result<KRefPtr<core::vfs::DirectoryEntry>> follow(KRefPtr<core::vfs::DirectoryEntry> start,
                                                               gen::String path) {
	if(!start) {
		return core::Error::InvalidArgument;
	}

	auto dentry = start;
	do {
		auto [component, new_path] = pop_component(path);
		//  One of the path components does not exist
		if(dentry->type == core::vfs::DirectoryEntry::Type::Negative) {
			//  FIXME: Add errors for FS
			return core::Error::InvalidArgument;
		}

		assert(dentry->inode);
		//  Dentry is positive, now check if the inode is actually a directory
		if(dentry->inode->type() != core::vfs::InodeType::Directory) {
			//  FIXME: Add errors for FS
			return core::Error::InvalidArgument;
		}

		auto new_dentry = core::vfs::dentry_alloc();
		if(!new_dentry) {
			return core::Error::NoMem;
		}
		new_dentry->name = component;

		auto directory_inode = static_cast<KRefPtr<core::vfs::Directory>>(dentry->inode);
		auto result = directory_inode->lookup(component);
		//  An error occured
		if(!result) {
			log.error("Directory inode lookup failed: {}", static_cast<size_t>(result.error()));
			return core::Error::IOFail;
		}

		new_dentry->inode = result.destructively_move_data();
		//  No error, but the inode just doesn't exist
		if(!new_dentry->inode) {
			new_dentry->type = core::vfs::DirectoryEntry::Type::Negative;
		}

		dentry = new_dentry;
		path = new_path;
	} while(!path.empty());

	return dentry;
}

core::Error core::vfs::init() {
	s_filesystem = KHeap::make<Ramfs>();

	auto result = s_filesystem->mount();
	if(!result) {
		::log.fatal("Mounting root filesystem failed: {}", static_cast<size_t>(result.error()));
		kpanic();
	}
	s_root = result.destructively_move_data();

	auto name = s_filesystem->name();
	::log.info("Mounted root filesystem: {}", name.data());

	return core::Error::Ok;
}
