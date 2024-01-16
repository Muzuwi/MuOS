#include "Core/Error/Error.hpp"
#include "Ext2FS.hpp"

using namespace core::fs::ext2;

core::Result<KRefPtr<core::vfs::DirectoryEntry>> Ext2Fs::mount() {
	return core::Error::IOFail;
}
