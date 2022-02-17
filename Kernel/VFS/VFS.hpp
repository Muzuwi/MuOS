#pragma once
#include <LibGeneric/SharedPtr.hpp>

class Dentry;
class Inode;

class VFS {
	gen::SharedPtr<Dentry> m_root_dentry;

	VFS() = default;

	gen::SharedPtr<Dentry> path_lookup(gen::String path);
public:
	static VFS& instance() {
		static VFS vfs {};
		return vfs;
	}

	void set_root_inode(gen::SharedPtr<Inode>);
};