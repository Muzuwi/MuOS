#pragma once
#include <LibGeneric/List.hpp>
#include <LibGeneric/SharedPtr.hpp>
#include <LibGeneric/String.hpp>

class Inode;

class Dentry {
	gen::String m_name;
	gen::SharedPtr<Inode> m_inode;
	gen::SharedPtr<Dentry> m_parent;
	gen::List<gen::SharedPtr<Dentry>> m_children;
public:
	Dentry(gen::String name, gen::SharedPtr<Inode> inode,
	       gen::SharedPtr<Dentry> parent = gen::SharedPtr<Dentry> { nullptr });

	//	gen::String const& name() const { return m_name; }
	//	gen::SharedPtr<Inode> inode() const { return m_inode; }
	//	gen::SharedPtr<Dentry> parent() const { return m_parent; }
};