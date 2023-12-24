#pragma once
#include <Core/Error/Error.hpp>
#include <Core/Log/Logger.hpp>
#include <LibGeneric/Spinlock.hpp>

namespace vgacon {
	core::Error init();

	class VgaConsole : public core::log::Sink {
	public:
		constexpr VgaConsole() = default;

		void push(core::log::LogLevel, const char* tag, const char* message) override;
		void clear();
	private:
		gen::Spinlock m_lock {};
	};
}
