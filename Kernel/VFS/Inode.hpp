#pragma once
#include <LibGeneric/SharedPtr.hpp>
#include <LibGeneric/String.hpp>
#include <Structs/KOptional.hpp>

enum class InodeType {
	File,
	Directory
};

class Inode {
	InodeType m_type;
public:
	Inode(InodeType);

	InodeType type() const { return m_type; }

	virtual gen::SharedPtr<Inode> lookup(gen::String) = 0;
};
