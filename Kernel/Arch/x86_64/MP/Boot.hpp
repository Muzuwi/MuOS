#pragma once
#include <Arch/VM.hpp>

class Thread;
namespace core::mp {
	class Environment;
}

extern "C" {
	void _ap_entrypoint(core::mp::Environment* env, Thread* idle_task, PhysAddr code_page, PhysAddr data_page);
}

namespace arch::mp {
	void boot_aps();
}
