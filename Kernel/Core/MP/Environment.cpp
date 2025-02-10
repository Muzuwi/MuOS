#include <Core/MP/MP.hpp>
#include <LibGeneric/LockGuard.hpp>
#include <LibGeneric/Spinlock.hpp>
#include <stddef.h>

static constexpr const size_t max_supported_nodes = 32;
static core::mp::Environment s_environments[max_supported_nodes] {};
static constinit size_t s_next_env {};
static constinit gen::Spinlock s_lock {};

core::mp::Environment* core::mp::create_environment() {
	gen::LockGuard lock { s_lock };

	if(s_next_env >= max_supported_nodes) {
		return nullptr;
	}
	auto* env = &s_environments[s_next_env];
	env->node_id = s_next_env;
	s_next_env += 1;

	return env;
}
