#pragma once
#include <Core/Error/Error.hpp>
#include <SystemTypes.hpp>

namespace arch::rv64 {
	static constexpr int64 SBI_FID_LEGACY = 0x0;
	static constexpr int64 SBI_EID_PUTCHAR = 0x1;

	///  Initialize early logging console via OpenSBI calls.
	///
	///  This can be used for early logging available even before
	///  the kernel performs *any* initialization of its own. It
	///  shouldn't be used for regular logging, as the SBI console
	///  WILL block if a character cannot be written.
	core::Error init_sbi_earlycon();
}
