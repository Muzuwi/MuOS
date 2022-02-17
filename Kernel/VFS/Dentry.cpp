#include <VFS/Dentry.hpp>
#include <VFS/Inode.hpp>

Dentry::Dentry(gen::String name, gen::SharedPtr<Inode> inode, gen::SharedPtr<Dentry> parent)
    : m_name(gen::move(name))
    , m_inode(gen::move(inode))
    , m_parent(gen::move(parent))
    , m_children() {}
