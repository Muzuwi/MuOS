#pragma once

#ifdef __is_kernel_build__
#	include "MuOS/Assert.hpp"
#else
#	include "Native/Assert.hpp"
#endif
