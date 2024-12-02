#include <Arch/VM.hpp>
#include <Core/Error/Error.hpp>

core::Result<arch::PagingHandle> arch::addralloc() {
	return core::Result<arch::PagingHandle> { core::Error::Unsupported };
}

core::Error arch::addrfree(PagingHandle) {
	return core::Error::Unsupported;
}

core::Result<arch::PagingHandle> arch::addrclone(PagingHandle) {
	return core::Result<arch::PagingHandle> { core::Error::Unsupported };
}

core::Error arch::addrmap(PagingHandle, void*, void*, PageFlags) {
	return core::Error::Unsupported;
}

core::Error arch::addrunmap(PagingHandle, void*) {
	return core::Error::Unsupported;
}
