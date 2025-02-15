#pragma once
#include <Core/Error/Error.hpp>
#include <Core/Log/Formatters.hpp>
#include <LibFormat/Format.hpp>
#include <LibGeneric/Algorithm.hpp>
#include <LibGeneric/LockGuard.hpp>
#include <LibGeneric/Move.hpp>
#include <LibGeneric/Spinlock.hpp>
#include <LibGeneric/String.hpp>
#include <SystemTypes.hpp>

namespace core::log {
	enum class LogLevel {
		Debug = 0,
		Info,
		Warning,
		Error,
		Fatal
	};

	class Sink {
	public:
		/** Push log data to a sink
		 *  Expected lifetime of passed in tag/message pointers is limited to
		 *  only the call to the sink's push method. If required, the sink must
		 *  allocate a heap buffer (for example, if the output happens asynchronously).
		 */
		virtual void push(LogLevel, char const* tag, char const* message) = 0;
	};

	/**	Register a system logger sink.
	 *	This can be used to register a new sink that all kernel log output
	 * 	is sent to.
	 */
	core::Error register_sink(Sink*);

	/**	INTERNAL: Push data to all kernel logger sinks
	 */
	void _push(LogLevel, char const* tag, char const* message);

	class Logger {
	public:
		constexpr Logger(char const* tag, LogLevel level)
		    : m_tag(tag)
		    , m_minimum_level(level) {}

		template<typename... Args>
		void debug(char const* format, Args... args) {
			format_and_push(LogLevel::Debug, format, gen::forward<Args>(args)...);
		}

		template<typename... Args>
		void info(char const* format, Args... args) {
			format_and_push(LogLevel::Info, format, gen::forward<Args>(args)...);
		}

		template<typename... Args>
		void warning(char const* format, Args... args) {
			format_and_push(LogLevel::Warning, format, gen::forward<Args>(args)...);
		}

		template<typename... Args>
		void error(char const* format, Args... args) {
			format_and_push(LogLevel::Error, format, gen::forward<Args>(args)...);
		}

		template<typename... Args>
		void fatal(char const* format, Args... args) {
			format_and_push(LogLevel::Fatal, format, gen::forward<Args>(args)...);
		}

		/** Log with specified LogLevel
		 *  This only exists to provide a nicer interface for logging using computed
		 *  LogLevel values (not known at compile time, for example).
		 */
		template<typename... Args>
		void log(LogLevel level, char const* format, Args... args) {
			format_and_push(level, format, gen::forward<Args>(args)...);
		}
	private:
		static constexpr const size_t internal_buffer_size = 256;

		template<typename... Args>
		void format_and_push(LogLevel level, char const* format, Args... args) {
			char storage[internal_buffer_size];

			if(static_cast<size_t>(level) < static_cast<size_t>(m_minimum_level)) {
				return;
			}

			Format::format(format, storage, sizeof(storage), gen::forward<Args>(args)...);
			core::log::_push(level, m_tag, storage);
		}

		char const* m_tag {};
		LogLevel m_minimum_level {};
	};

	inline constexpr Logger create_logger(char const* tag, LogLevel default_log_level) {
		return { tag, default_log_level };
	}
}

#define CREATE_LOGGER(tag, minimum_level) static constinit auto log = ::core::log::create_logger(tag, minimum_level)
