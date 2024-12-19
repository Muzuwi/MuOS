#pragma once
#include <Arch/MP.hpp>
#include <SystemTypes.hpp>

class Thread;

namespace core::task {
	struct Task;
}

namespace core::mp {
	struct Environment {
		uint64 node_id;
		core::task::Task* task;

		constexpr core::task::Task* current_task() { return task; }
		constexpr void set_task(core::task::Task* task) { this->task = task; }
	};

	Environment* create_environment();
	[[noreturn]] void bootstrap_this_node(Thread* idle_task = nullptr, Thread* init = nullptr);
}

static inline core::mp::Environment* this_cpu() {
	return static_cast<core::mp::Environment*>(arch::mp::environment_get());
}
