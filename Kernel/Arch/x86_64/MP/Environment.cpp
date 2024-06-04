#include <Core/Assert/Assert.hpp>
#include <LibGeneric/StaticVector.hpp>
#include <stddef.h>
#include "ExecutionEnvironment.hpp"
#include "LibGeneric/LockGuard.hpp"
#include "LibGeneric/Spinlock.hpp"

static constexpr const size_t max_supported_cpus = 32;
static arch::mp::ExecutionEnvironment s_environments[max_supported_cpus] {};
static size_t s_next_env {};
static gen::Spinlock s_lock {};

arch::mp::ExecutionEnvironment* arch::mp::create_environment() {
	gen::LockGuard lock { s_lock };

	if(s_next_env >= max_supported_cpus) {
		return nullptr;
	}
	auto* env = &s_environments[s_next_env];
	s_next_env += 1;

	ENSURE(env == env->self_reference);
	return env;
}
