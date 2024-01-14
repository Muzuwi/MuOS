#pragma once
#include <Core/Error/Error.hpp>
#include <Core/Object/Object.hpp>
#include <LibGeneric/List.hpp>
#include <LibGeneric/Move.hpp>
#include <LibGeneric/String.hpp>

//  Just so it's more obvious which places need to be refcounted
template<typename T>
using KRefPtr = T*;

namespace core::vfs {
	enum class InodeType {
		File,
		Directory,
	};

	class Inode {
	public:
		constexpr Inode(InodeType type)
		    : m_type(type) {}

		[[nodiscard]] constexpr InodeType type() const { return m_type; }
	private:
		const InodeType m_type;
	};

	//  ======================================

	struct DirectoryEntry {
		//	Path component
		gen::String name;
		//  State of the dentry
		enum class Type {
			Positive,//  the associated inode exists on the fs
			Negative,//  the associated inode does not exist on the fs
		} type;
		//  Inode
		KRefPtr<Inode> inode;
	};

	//  ======================================

	class FileSystem : public obj::KObject {
	public:
		constexpr FileSystem(gen::String identifier)
		    : obj::KObject(obj::ObjectType::FileSystem, gen::move(identifier)) {}

		virtual core::Result<KRefPtr<DirectoryEntry>> mount() = 0;
	};

	//  ======================================

	class File : public Inode {
	public:
		constexpr File()
		    : Inode(InodeType::File) {}
	};

	class Directory : public Inode {
	public:
		constexpr Directory()
		    : Inode(InodeType::Directory) {}

		virtual core::Result<KRefPtr<Inode>> lookup(gen::String) = 0;
	};
}
