#pragma once

namespace gen {
	template<typename LockType>
	class LockGuard {
		LockType& m_lock;
	public:
		constexpr explicit LockGuard(LockType& lock) noexcept
		    : m_lock(lock) {
			m_lock.lock();
		}

		constexpr LockGuard(LockGuard&& lg) noexcept
		    : m_lock(lg) {}

		constexpr LockGuard(LockGuard const&) = delete;

		constexpr ~LockGuard() noexcept { m_lock.unlock(); }
	};
}