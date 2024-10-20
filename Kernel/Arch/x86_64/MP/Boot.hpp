#pragma once
#include <Arch/VM.hpp>

class Thread;
namespace arch::mp {
	class ExecutionEnvironment;
}

extern "C" {
	void _ap_entrypoint(arch::mp::ExecutionEnvironment* env, Thread* idle_task, PhysAddr code_page, PhysAddr data_page);
}

namespace arch::mp {
	void boot_aps();
}
