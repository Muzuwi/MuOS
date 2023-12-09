#pragma once

namespace core {
	/**	Common kernel error enumeration.
	 */
	enum class [[nodiscard("functions returning core::Error must be checked for failures")]] Error {
		Ok = 0,
		NoMem,
		IOFail,
		InvalidArgument,
	};
}
