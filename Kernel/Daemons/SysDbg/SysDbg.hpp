#pragma once
#include <LibGeneric/List.hpp>
#include <LibGeneric/SharedPtr.hpp>
#include <LibGeneric/String.hpp>
#include <SystemTypes.hpp>

class Process;

namespace SysDbg {
	void handle_command(gen::List<gen::String> const& args);
	void dump_process(gen::SharedPtr<Process> process, size_t depth = 0);

	[[noreturn]] void sysdbg_thread();
}
