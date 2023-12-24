#pragma once
#include <Core/Log/Logger.hpp>
#include <LibFormat/Format.hpp>
#include <LibGeneric/Spinlock.hpp>
#include <Memory/KHeap.hpp>
#include "LibGeneric/Move.hpp"

class LegacyKernelLogger {
public:
	static core::log::Logger& instance() {
		static core::log::Logger s_logger = core::log::create_logger("core::debug", core::log::LogLevel::Debug);
		return s_logger;
	}
};

#define DEPRECATED_LOGGER \
	[[deprecated("klogf-family functions are deprecated, use the new core::log::Logger interface")]]

/*
 *  Logs a message to the kernel debugger.
 * 	This is a legacy function - new code should use the core::log::Logger
 * 	interface instead.
 */
template<typename... Args>
DEPRECATED_LOGGER constexpr void klogf(char const* format, Args... args) {
	LegacyKernelLogger::instance().info(format, gen::forward<Args>(args)...);
}

/*
 *  Logs a message to the kernel debugger.
 * 	This is a legacy function - new code should use the core::log::Logger
 * 	interface instead.
 */
template<typename... Args>
DEPRECATED_LOGGER constexpr void klogf_static(char const* format, Args... args) {
	LegacyKernelLogger::instance().info(format, gen::forward<Args>(args)...);
}

/*
 *  Logs an error message to the kernel debugger.
 * 	This is a legacy function - new code should use the core::log::Logger
 * 	interface instead.
 */
template<typename... Args>
DEPRECATED_LOGGER constexpr void kerrorf(char const* format, Args... args) {
	LegacyKernelLogger::instance().error(format, gen::forward<Args>(args)...);
}

/*
 *  Logs an error message to the kernel debugger (using a static buffer with locking for formatting the message)
 * 	This is a legacy function - new code should use the core::log::Logger
 * 	interface instead.
 */
template<typename... Args>
DEPRECATED_LOGGER constexpr void kerrorf_static(char const* format, Args... args) {
	LegacyKernelLogger::instance().error(format, gen::forward<Args>(args)...);
}
