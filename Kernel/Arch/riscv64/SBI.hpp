#pragma once
#include <SystemTypes.hpp>

namespace arch::rv64 {
	using SbiArg = uint64;
	using SbiEid = int64;
	using SbiFid = int64;

	///  Error status returned by SBI calls.
	enum class SbiError : int64 {
		Success = 0,
		Failed = -1,
		NotSupported = -2,
		InvalidParam = -3,
		Denied = -4,
		InvalidAddress = -5,
		AlreadyAvailable = -6,
		AlreadyStarted = -7,
		AlreadyStopped = -8
	};

	///  Result of an SBI call.
	struct SbiResult {
		SbiError error;
		long value;

		//  Check if the call was successful.
		[[nodiscard]] constexpr bool ok() const { return error == SbiError::Success; }

		//  Ignore error state.
		constexpr void ignore_error() const {}
	};

	///  Perform an SBI call.
	///
	///  This can be used to call SBI functions. Each argument provided to this function
	///  directly matches to the a0-a7 registers. The last two arguments of the function
	///  correspond to the function ID and extension ID respectively.
	[[nodiscard]] __attribute__((no_stack_protector, naked)) inline SbiResult sbi_call(SbiArg, SbiArg, SbiArg, SbiArg,
	                                                                                   SbiArg, SbiArg, SbiFid, SbiEid) {
		asm volatile(
		        //  The calling convention for RISC-V specifies that each argument will
		        //  be passed starting from a0-a7. `ecall` expects the same convention
		        //  to be followed, so there's nothing to do. The output of an `ecall`
		        //	also matches nicely to the return value, so everything should just work.
		        //
		        //  `eid` and `fid` should be passed as the last two arguments to this function.
		        "ecall  \n"
		        "ret    \n");
	}

}