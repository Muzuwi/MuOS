#pragma once
#include <Core/Error/Error.hpp>
#include "Inode.hpp"

namespace core::vfs {
	core::Error init();

	DirectoryEntry* dentry_alloc();
	void dentry_free(DirectoryEntry*);
}
