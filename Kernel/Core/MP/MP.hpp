#pragma once
#include <Arch/MP.hpp>
#include <SystemTypes.hpp>
#ifdef ARCH_IS_x86_64
#	include <Arch/x86_64/MP/ExecutionEnvironment.hpp>
#endif

class Thread;
class Scheduler;

namespace core::mp {
	struct Environment {
#ifdef ARCH_IS_x86_64
		arch::mp::ExecutionEnvironment platform;
#endif
		Thread* thread;
		Scheduler* scheduler;
		uint64 node_id;

		constexpr Thread* current_thread() { return thread; }

		constexpr void set_thread(Thread* thread) { this->thread = thread; }
	};

	Environment* create_environment();
	[[noreturn]] void bootstrap_this_node(Thread* idle_task = nullptr, Thread* init = nullptr);
}

static inline core::mp::Environment* this_cpu() {
	return static_cast<core::mp::Environment*>(arch::mp::environment_get());
}
