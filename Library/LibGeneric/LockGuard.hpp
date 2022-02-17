#pragma once

namespace gen {
	template<typename LockType>
	class LockGuard {
		LockType& m_lock;
	public:
		LockGuard(LockType& lock) noexcept
		    : m_lock(lock) {
			m_lock.lock();
		}

		~LockGuard() noexcept { m_lock.unlock(); }

		LockGuard(LockGuard const&) = delete;
	};
}