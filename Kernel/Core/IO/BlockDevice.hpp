#pragma once
#include <Core/Object/Object.hpp>
#include <Structs/KFunction.hpp>
#include "Core/Error/Error.hpp"
#include "LibGeneric/String.hpp"
#include "SystemTypes.hpp"

namespace core::io {
	using block_t = size_t;
	using block_count_t = size_t;

	using SignatureBlockDeviceRead = core::Error(char*, size_t, block_t, block_count_t);
	using SignatureBlockDeviceWrite = core::Error(char const*, size_t, block_t, block_count_t);
	using SignatureBlockDeviceBlockSize = core::Error(size_t&);
	using SignatureBlockDeviceBlockCount = core::Error(block_count_t&);

	struct BlockDevice : public core::obj::KObject {
		constexpr BlockDevice(gen::String name)
		    : KObject(core::obj::ObjectType::BlockDevice, name) {}

		KFunction<SignatureBlockDeviceRead> read {};
		KFunction<SignatureBlockDeviceWrite> write {};
		KFunction<SignatureBlockDeviceBlockSize> blksize {};
		KFunction<SignatureBlockDeviceBlockCount> blkcount {};
	};
}
