#include <VFS/Dentry.hpp>
#include <VFS/Inode.hpp>
#include <VFS/VFS.hpp>

void VFS::set_root_inode(gen::SharedPtr<Inode> inode) {
	//	m_root_dentry = gen::make_shared<Dentry>("/", inode);
}

gen::SharedPtr<Dentry> VFS::path_lookup(gen::String path) {
	return gen::SharedPtr<Dentry>();
}
