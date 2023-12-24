#include <Core/Error/Error.hpp>
#include <Core/Log/Logger.hpp>
#include <LibGeneric/Algorithm.hpp>
#include <LibGeneric/LockGuard.hpp>
#include <LibGeneric/Spinlock.hpp>
#include <LibGeneric/StaticVector.hpp>

static const size_t max_logger_sinks = 4;
static gen::Spinlock s_lock {};
static gen::StaticVector<core::log::Sink*, max_logger_sinks> s_sinks {};

core::Error core::log::register_sink(Sink* sink) {
	if(!sink) {
		return core::Error::InvalidArgument;
	}

	gen::LockGuard lg { s_lock };
	if(gen::find(s_sinks, sink) != s_sinks.end()) {
		return core::Error::InvalidArgument;
	}
	if(!s_sinks.push_back(sink)) {
		return core::Error::InvalidArgument;
	}
	return core::Error::Ok;
}

void core::log::_push(LogLevel level, const char* tag, const char* message) {
	gen::LockGuard lg { s_lock };

	for(auto* sink : s_sinks) {
		sink->push(level, tag, message);
	}
}
