#pragma once

#include <LibGeneric/SharedPtr.hpp>
#include <SystemTypes.hpp>

class Process;

namespace SysDbg {
	void dump_process(gen::SharedPtr<Process> process, size_t depth = 0);

	[[noreturn]] void sysdbg_thread();
}
