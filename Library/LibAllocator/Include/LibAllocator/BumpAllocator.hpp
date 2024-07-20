#pragma once
#include <stddef.h>
#include <stdint.h>
#include "Arena.hpp"

namespace liballoc {
	class BumpAllocator {
	public:
		BumpAllocator(Arena arena) noexcept
		    : m_arena(arena)
		    , m_current(arena.base) {}

		void* allocate(size_t size) {
			if(reinterpret_cast<void*>(reinterpret_cast<uintptr_t>(m_current) + size) > end()) {
				return nullptr;
			}

			auto val = m_current;
			m_current = reinterpret_cast<void*>(reinterpret_cast<uintptr_t>(m_current) + size);
			return val;
		}

		[[nodiscard]] constexpr void* start() const { return m_arena.base; }

		[[nodiscard]] void* end() const { return m_arena.end(); }

		[[nodiscard]] constexpr void* current() const { return m_current; }

		[[nodiscard]] constexpr size_t size() const { return m_arena.length; }
	private:
		Arena m_arena;
		void* m_current;
	};
}