#pragma once
#include <Structs/KResult.hpp>

namespace core {
	/**	Common kernel error enumeration.
	 */
	enum class [[nodiscard("functions returning core::Error must be checked for failures")]] Error {
		Ok = 0,
		NoMem,
		IOFail,
		InvalidArgument,
		EntityAlreadyExists,
		EntityMissing,
		Unsupported,
	};

	template<typename T>
	using Result = KResult<T, Error>;
}
