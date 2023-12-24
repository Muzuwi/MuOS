#pragma once
#include <Core/Error/Error.hpp>
#include <Core/Log/Logger.hpp>

namespace serialcon {
	core::Error init();

	class SerialConsole : public core::log::Sink {
	public:
		constexpr SerialConsole() = default;

		void push(core::log::LogLevel, const char* tag, const char* message) override;
	private:
		gen::Spinlock m_lock {};
	};
}
